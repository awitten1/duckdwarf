#pragma once

#include "duckdb.hpp"
#include "duckdb/storage/storage_extension.hpp"

namespace duckdb {

class DwarfExtension : StorageExtension {
public:
	void Load(ExtensionLoader &db);
	std::string Name() {
		return "dwarf";
	}
	std::string Version() const;
};

class QuackExtension : public Extension {
public:
	void Load(ExtensionLoader &db) override;
	std::string Name() override {
		return "dwarf";
	}
	std::string Version() const override;
};

} // namespace duckdb
