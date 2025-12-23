
#include "duckdb/common/helper.hpp"
#include "duckdb/common/vector_size.hpp"
#include "duckdb/function/function.hpp"
#include "duckdb/parser/parsed_data/create_macro_info.hpp"
#include "dwarf.hpp"
#define DUCKDB_EXTENSION_MAIN

#include "dwarf_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/function/scalar_function.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>
#include "duckdb/common/types.hpp"
#include "duckdb/common/unique_ptr.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/main/client_context.hpp"

namespace duckdb {

struct DwarfFunctionData : public FunctionData {
	explicit DwarfFunctionData(string dwarf_file) : dwarf_file(std::move(dwarf_file)) {
	}

	unique_ptr<FunctionData> Copy() const override {
		return make_uniq<DwarfFunctionData>(dwarf_file);
	}
	bool Equals(const FunctionData &other) const override {
		return dynamic_cast<const DwarfFunctionData &>(other).dwarf_file == dwarf_file;
	}

	string dwarf_file;
};

static unique_ptr<FunctionData> BindDwarfFunction(ClientContext &context, TableFunctionBindInput &input,
                                                  vector<LogicalType> &return_types, vector<string> &names) {

	return_types.push_back(LogicalType::VARCHAR);
	names.push_back("dwarf_tag");

	return_types.push_back(LogicalType::BIGINT);
	names.push_back("dwarf_offset");

	return_types.push_back(LogicalType::VARCHAR);
	names.push_back("dwarf_section");

	return_types.push_back(LogicalType::BIGINT);
	names.push_back("parent_dwarf_offset");

	return_types.push_back(LogicalType::LIST(LogicalType::STRUCT({{"dwarf_attr_key", LogicalType::VARCHAR},
	                                                              {"dwarf_attr_value", LogicalType::VARCHAR},
	                                                              {"dwarf_attr_form", LogicalType::VARCHAR}})));
	names.push_back("dwarf_attributes");

	auto dwarf_file = input.inputs[0].ToString();

	return make_uniq<DwarfFunctionData>(dwarf_file);
}

struct DwarfFileState : public GlobalTableFunctionState {
	explicit DwarfFileState(const DwarfFunctionData &input) : dw(input.dwarf_file), it(dw.begin()) {
	}
	Dwarf dw;
	Dwarf::iterator it;
};

inline void DwarfTableFunction(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	auto &dwarf_state = dynamic_cast<DwarfFileState &>(*data.global_state);
	auto &it = dwarf_state.it;
	auto &dw = dwarf_state.dw;
	if (it == dw.end()) {
		return;
	}

	int row = 0;
	for (; row < STANDARD_VECTOR_SIZE && it != dw.end(); ++row, ++it) {
		Dwarf::Die d = *it;
		output.SetValue(0, row, d.tag);
		output.SetValue(1, row, Value(int64_t(d.offset)));
		output.SetValue(2, row, d.section);
		output.SetValue(3, row, d.parent_die != -1 ? Value(int64_t(d.parent_die)) : Value());

		vector<Value> attrs;
		for (const auto &attr : d.attributes) {
			auto s = Value::STRUCT(
			    {{"dwarf_attr_key", attr.name}, {"dwarf_attr_value", attr.value}, {"dwarf_attr_form", attr.form}});
			attrs.push_back(s);
		}
		output.SetValue(4, row,
		                Value::LIST(LogicalType::STRUCT({{"dwarf_attr_key", LogicalType::VARCHAR},
		                                                 {"dwarf_attr_value", LogicalType::VARCHAR},
		                                                 {"dwarf_attr_form", LogicalType::VARCHAR}}),
		                            attrs));
	}
	output.SetCardinality(row);
}

unique_ptr<GlobalTableFunctionState> InitDwarfState(ClientContext &context, TableFunctionInitInput &input) {
	auto &bind_data = dynamic_cast<const DwarfFunctionData &>(*input.bind_data);
	return make_uniq<DwarfFileState>(bind_data);
}

static void LoadInternal(ExtensionLoader &loader) {

	TableFunction t("dwarf_info", {duckdb::LogicalType::VARCHAR}, DwarfTableFunction, BindDwarfFunction,
	                InitDwarfState);

	loader.RegisterFunction(t);
}

void DwarfExtension::Load(ExtensionLoader &loader) {
	LoadInternal(loader);
}

std::string DwarfExtension::Name() {
	return "dwarf";
}

std::string DwarfExtension::Version() const {
#ifdef EXT_VERSION_DWARF
	return EXT_VERSION_DWARF;
#else
	return "";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_CPP_EXTENSION_ENTRY(dwarf, loader) {
	duckdb::LoadInternal(loader);
}
}
