#pragma once

#include <functional>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstring>

// supposed to include dwarf.h before libdwarf.h
#include "dwarf.h"
#include "libdwarf.h"

using namespace std;

class Dwarf {
public:
	struct Attribute {
		std::string name;
		std::string form;
		std::string value;
	};

	struct Die {
		std::string tag;
		Dwarf_Off offset;
		Dwarf_Off parent_die;
		std::string section;
		std::vector<Attribute> attributes;
	};

	class iterator {
		std::vector<Dwarf_Die> die_stack_;
		Dwarf_Debug dbg_;
		bool is_end_ = false;
		Dwarf_Unsigned next_cu_header_offset_ = 0;

	public:
		explicit iterator(bool is_end) : is_end_(is_end) {
		}
		explicit iterator(Dwarf_Debug dbg) : dbg_(dbg), is_end_(false) {
			Dwarf_Unsigned cu_header_length = 0;
			Dwarf_Half version_stamp = 0;
			Dwarf_Off abbrev_offset = 0;
			Dwarf_Half address_size = 0;
			Dwarf_Half length_size = 0;
			Dwarf_Half extension_size = 0;
			Dwarf_Sig8 type_signature = {0};
			Dwarf_Unsigned typeoffset = 0;
			Dwarf_Half header_cu_type = 0;
			Dwarf_Error error;

			Dwarf_Die first_die;
			int res = dwarf_next_cu_header_e(dbg_, true, &first_die, &cu_header_length, &version_stamp,
			                                 &abbrev_offset, &address_size, &length_size, &extension_size,
			                                 &type_signature, &typeoffset, &next_cu_header_offset_, &header_cu_type,
			                                 &error);

			if (res != DW_DLV_OK) {
				is_end_ = true;
				return;
			}

			die_stack_.push_back(first_die);
		}

		~iterator() {
			for (auto die : die_stack_) {
				dwarf_dealloc_die(die);
			}
		}

		Die operator*() {
			Die cppDie;

			auto &current_die = die_stack_.back();
			cppDie.tag = Dwarf::GetTagName(current_die);

			Dwarf_Error error;
			dwarf_dieoffset(current_die, &cppDie.offset, &error);

			if (die_stack_.size() > 1) {
				dwarf_dieoffset(die_stack_[die_stack_.size() - 2], &cppDie.parent_die, &error);
			} else {
				cppDie.parent_die = -1;
			}

			const char *sec_name = nullptr;
			dwarf_get_die_section_name_b(current_die, &sec_name, &error);
			cppDie.section = sec_name ? sec_name : "";

			Dwarf::GetDieAttributes(dbg_, current_die, cppDie.attributes);
			return cppDie;
		}

		iterator &operator++() {
			if (is_end_ || die_stack_.empty()) {
				is_end_ = true;
				return *this;
			}

			Dwarf_Error error = 0;
			auto current_die = die_stack_.back();

			Dwarf_Die child_die = 0;
			int res = dwarf_child(current_die, &child_die, &error);
			if (res == DW_DLV_OK) {
				die_stack_.push_back(child_die);
				return *this;
			}

			Dwarf_Die sibling_die = 0;
			res = dwarf_siblingof_c(current_die, &sibling_die, &error);
			if (res == DW_DLV_OK) {
				dwarf_dealloc_die(current_die);
				die_stack_.back() = sibling_die;
				return *this;
			}

			dwarf_dealloc_die(current_die);
			die_stack_.pop_back();

			while (!die_stack_.empty()) {
				auto parent_die = die_stack_.back();

				Dwarf_Die parent_sibling = 0;
				res = dwarf_siblingof_c(parent_die, &parent_sibling, &error);
				if (res == DW_DLV_OK) {
					dwarf_dealloc_die(parent_die);
					die_stack_.back() = parent_sibling;
					return *this;
				}
				dwarf_dealloc_die(parent_die);
				die_stack_.pop_back();
			}

			Dwarf_Unsigned cu_header_length = 0;
			Dwarf_Half version_stamp = 0;
			Dwarf_Off abbrev_offset = 0;
			Dwarf_Half address_size = 0;
			Dwarf_Half length_size = 0;
			Dwarf_Half extension_size = 0;
			Dwarf_Sig8 type_signature = {0};
			Dwarf_Unsigned typeoffset = 0;
			Dwarf_Half header_cu_type = 0;

			Dwarf_Die next_cu_die;
			res = dwarf_next_cu_header_e(dbg_, true, &next_cu_die, &cu_header_length, &version_stamp,
			                             &abbrev_offset, &address_size, &length_size, &extension_size, &type_signature,
			                             &typeoffset, &next_cu_header_offset_, &header_cu_type, &error);

			if (res == DW_DLV_OK) {
				die_stack_.push_back(next_cu_die);
				return *this;
			}

			is_end_ = true;
			return *this;
		}

		bool operator!=(const iterator &other) const {
			return is_end_ != other.is_end_;
		}

		bool operator==(const iterator &other) const {
			return is_end_ == other.is_end_;
		}
	};

	explicit Dwarf(const std::string &filename) : filename_(filename) {
		Dwarf_Error error = 0;
		char true_path_out[4096];
		Dwarf_Debug dbg;

		int res = dwarf_init_path(filename_.c_str(), true_path_out, sizeof(true_path_out), DW_GROUPNUMBER_ANY, NULL,
		                          NULL, &dbg, &error);

		if (res == DW_DLV_ERROR) {
			char *errmsg = dwarf_errmsg(error);
			std::string err_str = errmsg ? errmsg : "Unknown error";
			throw std::runtime_error("Dwarf initialization failed: " + err_str);
		}
		if (res == DW_DLV_NO_ENTRY) {
			throw std::runtime_error("File not found or not a supported object file: " + filename_);
		}
		dbg_ = dbg;
	}

	~Dwarf() {
		dwarf_finish(dbg_);
	}

	iterator begin() const {
		return iterator(dbg_);
	}

	iterator end() const {
		return iterator(true);
	}

private:
	std::string filename_;
	Dwarf_Debug dbg_;

	static void GetDieAttributes(Dwarf_Debug dbg, Dwarf_Die die, std::vector<Attribute> &out_attrs) {
		Dwarf_Attribute *attrbuf = 0;
		Dwarf_Signed attrcount = 0;
		Dwarf_Error error = 0;

		if (dwarf_attrlist(die, &attrbuf, &attrcount, &error) != DW_DLV_OK) {
			return;
		}

		for (int i = 0; i < attrcount; ++i) {
			Attribute attr;
			Dwarf_Half attr_num = 0;
			Dwarf_Half form_num = 0;

			dwarf_whatattr(attrbuf[i], &attr_num, &error);
			const char *name_ptr = nullptr;
			if (dwarf_get_AT_name(attr_num, &name_ptr) == DW_DLV_OK) {
				attr.name = name_ptr;
			} else {
				attr.name = "DW_AT_<unknown>";
			}

			dwarf_whatform(attrbuf[i], &form_num, &error);
			const char *form_ptr = nullptr;
			if (dwarf_get_FORM_name(form_num, &form_ptr) == DW_DLV_OK) {
				attr.form = form_ptr;
			} else {
				attr.form = "DW_FORM_<unknown>";
			}

			attr.value = GetAttributeValue(attrbuf[i], form_num);

			out_attrs.push_back(attr);
			dwarf_dealloc_attribute(attrbuf[i]);
		}
		dwarf_dealloc(dbg, attrbuf, DW_DLA_LIST);
	}

	static std::string GetAttributeValue(Dwarf_Attribute attr, Dwarf_Half form) {
		Dwarf_Error error = 0;
		int res;

		if (form == DW_FORM_string || form == DW_FORM_strp || form == DW_FORM_line_strp || form == DW_FORM_strx ||
		    form == DW_FORM_strx1 || form == DW_FORM_strx2 || form == DW_FORM_strx3 || form == DW_FORM_strx4 ||
		    form == DW_FORM_GNU_strp_alt) {

			char *stringval = nullptr;
			res = dwarf_formstring(attr, &stringval, &error);
			if (res == DW_DLV_OK && stringval) {
				return std::string(stringval);
			}
		}

		if (form == DW_FORM_data1 || form == DW_FORM_data2 || form == DW_FORM_data4 || form == DW_FORM_data8 ||
		    form == DW_FORM_udata) {

			Dwarf_Unsigned val = 0;
			res = dwarf_formudata(attr, &val, &error);
			if (res == DW_DLV_OK) {
				return std::to_string(val);
			}
		}

		if (form == DW_FORM_sdata) {
			Dwarf_Signed val = 0;
			res = dwarf_formsdata(attr, &val, &error);
			if (res == DW_DLV_OK) {
				return std::to_string(val);
			}
		}

		if (form == DW_FORM_addr) {
			Dwarf_Addr addr = 0;
			res = dwarf_formaddr(attr, &addr, &error);
			if (res == DW_DLV_OK) {
				stringstream ss;
				ss << "0x" << std::hex << addr;
				return ss.str();
			}
		}

		if (form == DW_FORM_flag || form == DW_FORM_flag_present) {
			Dwarf_Bool flag = 0;
			res = dwarf_formflag(attr, &flag, &error);
			if (res == DW_DLV_OK) {
				return flag ? "true" : "false";
			}
		}

		if (form == DW_FORM_ref_addr || form == DW_FORM_ref1 || form == DW_FORM_ref2 || form == DW_FORM_ref4 ||
		    form == DW_FORM_ref8 || form == DW_FORM_ref_udata || form == DW_FORM_sec_offset) {

			Dwarf_Off off = 0;
			res = dwarf_global_formref(attr, &off, &error);
			if (res == DW_DLV_OK) {
				stringstream ss;
				ss << "0x" << std::hex << off;
				return ss.str();
			}
		}

		if (form == DW_FORM_exprloc) {
			Dwarf_Unsigned expr_len = 0;
			Dwarf_Ptr block_ptr = nullptr;

			// dwarf_formexprloc returns the pointer and length
			res = dwarf_formexprloc(attr, &expr_len, &block_ptr, &error);
			if (res == DW_DLV_OK) {
				stringstream ss;
				ss << "(exprloc=" << block_ptr << ",len=" << expr_len << ")";
				return ss.str();
			}
		}

		return "(value extraction not implemented for this form)";
	}

	static std::string GetTagName(Dwarf_Die die) {
		Dwarf_Half tag_val;
		Dwarf_Error error;
		const char *tag_name = nullptr;

		if (dwarf_tag(die, &tag_val, &error) == DW_DLV_OK) {
			if (dwarf_get_TAG_name(tag_val, &tag_name) == DW_DLV_OK) {
				return std::string(tag_name);
			}
		}
		return "<unknown>";
	}
};
