#pragma once

#include "duckdb.hpp"
#include "duckdb/storage/storage_extension.hpp"

namespace duckdb {

class DwarfExtension : StorageExtension {

};

class QuackExtension : public Extension {
public:
	void Load(ExtensionLoader &db) override;
	std::string Name() override;
	std::string Version() const override;
};

} // namespace duckdb
