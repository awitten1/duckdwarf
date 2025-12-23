# DuckDB Dwarf Extension
A duckdb extension for reading DWARF information.  I'm still trying to think of interseting use cases.

## Building
### Managing dependencies
```
./install-libdwarf.sh
make reldebug
```

### Build steps
Now to build the extension, run:
```sh
make release
```
The main binaries that will be built are:
```sh
./build/release/duckdb
./build/release/test/unittest
./build/release/extension/<extension_name>/<extension_name>.duckdb_extension
```

## Running the extension
```
--- On Mac you might need to run dsymutil on ./build/reldebug/duckdb for this to work.
D from dwarf_info('./build/release/duckdb')
```

A more interesting query:
```
PIVOT (
  SELECT dwarf_tag, dwarf_offset, attr.dwarf_attr_key, attr.dwarf_attr_value
  FROM (
    SELECT *, unnest(dwarf_attributes) as attr
    FROM dwarf_info('./test_dwarf')
  )
  WHERE dwarf_tag = 'DW_TAG_class_type'
) ON dwarf_attr_key USING first(dwarf_attr_value);
```
