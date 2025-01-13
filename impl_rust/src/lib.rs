#![feature(vec_into_raw_parts)]

mod attribute;
mod flag;
mod parser;
mod results;
mod style;

use crate::attribute::Attribute;
use crate::flag::{Flag, FlagAction};
use crate::parser::{Parsed, Parser};
use crate::results::Error;
use crate::style::{Style, StyleGroup};
use enumflags2::BitFlag;
use libc::c_char;
use std::ffi::{c_int, CStr, CString};
use std::{ptr, slice};
use std::ops::{Deref, DerefMut};

#[repr(C)]
pub enum ArgpxStatus {
    Okay = 0,

    BadAttribute = 1,

    NotAllowedSparseAssign = 2,
    NotAllowedAssigner = 3,

    InvalidParameters = 4,
}

#[repr(C)]
pub struct ArgpxParseResult {
    args: *mut ArgpxString,
    args_count: usize,
}

#[repr(C)]
pub struct ArgpxStyleGroup {
    prefix: *const c_char,
    assigner: *const c_char,
    delimiter: *const c_char,
    attribute: u8,
}

#[repr(C)]
pub struct ArgpxFlag<'a> {
    group_id: usize,
    name: *const c_char,
    action: *mut FlagAction<'a>,
}

#[repr(transparent)]
pub struct ArgpxString(String);

impl From<String> for ArgpxString {
    fn from(value: String) -> Self {
        Self(value)
    }
}

impl Into<String> for ArgpxString {
    fn into(self) -> String {
        self.0
    }
}

unsafe fn c_argv_to_vec(
    argc: usize,
    argv: *const *mut c_char,
) -> Result<Vec<String>, &'static str> {
    let mut args = Vec::with_capacity(argc as usize);

    for i in 0..argc {
        let c_str = CStr::from_ptr(*argv.offset(i as isize));
        match c_str.to_str() {
            Ok(arg) => args.push(arg.to_string()),
            Err(_) => return Err("Invalid UTF-8 in argument"),
        }
    }

    Ok(args)
}

#[export_name = "hello"]
pub extern "C" fn hello() {
    println!("Hello World From Rust!!!")
}

#[export_name = "ArgpxParser_New"]
pub unsafe extern "C" fn create_parser(style: *mut *mut Style) -> *mut Parser<'static> {
    if style.is_null() {
        return ptr::null_mut();
    }
    let parser = Box::into_raw(Box::new(Parser::new(*Box::from_raw(*style))));
    *style = ptr::null_mut();
    parser
}

#[export_name = "ArgpxParser_AppendFlag"]
pub unsafe extern "C" fn append_flag<'a>(parser: &mut Parser<'a>, flag: ArgpxFlag<'a>) -> i32 {
    let name = match convert_nullable_c_str(flag.name) {
        Some(x) => x,
        None => return -1,
    };
    let action = *Box::from_raw(flag.action);
    parser.flag(Flag {
        name,
        group_id: flag.group_id,
        action,
    });
    0
}

#[export_name = "ArgpxParser_Parse"]
pub unsafe extern "C" fn parser_parse(
    parser: &mut Parser,
    args_count: usize,
    arg_ptr: *const *mut c_char,
    terminate_at: usize,
    out: &mut ArgpxParseResult,
) -> ArgpxStatus {
    let args = match c_argv_to_vec(args_count, arg_ptr) {
        Ok(args) => args,
        Err(_) => return ArgpxStatus::InvalidParameters,
    };

    let result = parser.parse(args, terminate_at);
    match result {
        Ok(parsed) => {
            let mut boxed = parsed.args.into_boxed_slice();
            let a = boxed.deref_mut().as_mut_ptr();
            *out = ArgpxParseResult {
                args: a as *mut ArgpxString,
                args_count: boxed.len(),
            };
            std::mem::forget(boxed);
            ArgpxStatus::Okay
        }
        Err(Error::BadAttribute(_)) => ArgpxStatus::BadAttribute,
        Err(Error::ArgpxNotAllowed(attr)) => match attr {
            Attribute::NoAssigner => ArgpxStatus::NotAllowedAssigner,
            Attribute::NoSparseAssign => ArgpxStatus::NotAllowedSparseAssign,
            _ => unreachable!(),
        },
    }
}

#[export_name = "ArgpxParser_Free"]
pub unsafe extern "C" fn free_parser(parser: *mut Parser) {
    drop(Box::from_raw(parser))
}

#[export_name = "ArgpxAction_NewSetBool"]
pub unsafe extern "C" fn set_bool_action(ptr: *mut bool) -> *mut FlagAction<'static> {
    let reference = match ptr.as_mut() {
        Some(x) => x,
        None => return ptr::null_mut(),
    };
    Box::into_raw(Box::new(FlagAction::SetBool(reference)))
}

#[export_name = "ArgpxAction_NewSetInt"]
pub unsafe extern "C" fn set_int_action(ptr: *mut i32) -> *mut FlagAction<'static> {
    let reference = match ptr.as_mut() {
        Some(x) => x,
        None => return ptr::null_mut(),
    };
    Box::into_raw(Box::new(FlagAction::SetInt(reference)))
}

#[export_name = "ArgpxString_New"]
pub unsafe extern "C" fn string_new() -> ArgpxString {
    String::new().into()
}

#[export_name = "ArgpxString_Length"]
pub unsafe extern "C" fn string_length(s: *const ArgpxString) -> usize {
    match s.as_ref() {
        Some(s) => s.0.len(),
        None => usize::MAX
    }
}

#[export_name = "ArgpxString_Read"]
pub unsafe extern "C" fn string_read_to_c(s: *mut *mut ArgpxString) -> *mut c_char {
    if s.is_null() {
        return ptr::null_mut();
    }
    if (*s).is_null() {
        return ptr::null_mut();
    }
    let string: String = (*s).read().into();
    *s = ptr::null_mut();
    match CString::new(string).ok() {
        Some(x) => x.into_raw(),
        None => ptr::null_mut(),
    }
}

#[export_name = "ArgpxAction_NewParamMulti"]
pub unsafe extern "C" fn params_multi_action(
    params: *mut String,
    count: usize,
) -> *mut FlagAction<'static> {
    let arr = slice::from_raw_parts_mut(params, count);
    Box::into_raw(Box::new(FlagAction::ParamMulti(arr)))
}

unsafe fn convert_nullable_c_str(ptr: *const c_char) -> Option<&'static str> {
    if ptr.is_null() {
        None
    } else {
        CStr::from_ptr(ptr).to_str().ok()
    }
}

#[export_name = "ArgpxStyle_New"]
pub extern "C" fn create_style() -> *mut Style {
    Box::into_raw(Box::new(Style::new()))
}

#[export_name = "ArgpxStyle_AppendGroup"]
pub unsafe extern "C" fn style_append_group(style: &mut Style, group: ArgpxStyleGroup) -> isize {
    let ArgpxStyleGroup {
        prefix,
        assigner,
        delimiter,
        attribute,
    } = group;

    let prefix = match convert_nullable_c_str(prefix) {
        Some(prefix) => prefix,
        None => return -1,
    };

    let assigner = convert_nullable_c_str(assigner);

    let delimiter = convert_nullable_c_str(delimiter);

    let attribute = match Attribute::from_bits(attribute) {
        Ok(attr) => attr,
        Err(_) => return -1,
    };

    style.group(StyleGroup::new(prefix, assigner, delimiter, attribute)) as isize
}

#[export_name = "ArgpxStyle_Free"]
pub unsafe extern "C" fn free_style(style: *mut Style) {
    drop(Box::from_raw(style))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        hello();
    }
}
