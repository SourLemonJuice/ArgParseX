use std::collections::HashMap;
use crate::style::StyleGroup;

pub struct ParamList(Vec<String>);

pub enum FlagAction<'a> {
    // get single string
    ParamSingle(&'a mut String),
    // get an array of strings
    ParamMulti(&'a mut [String]),
    // get a dynamic list of strings
    ParamList(&'a mut ParamList),
    // get a boolean
    SetBool(&'a mut bool),
    // get a 32-bit signed integer
    SetInt(&'a mut i32),
}

pub struct Flag<'a> {
    pub group_id: usize,
    pub name: &'a str,
    pub action: FlagAction<'a>
}

pub struct Flags<'a> {
    pub map: HashMap<StyleGroup, Vec<Flag<'a>>>
}

impl<'a> Flags<'a> {
    pub fn new() -> Self {
        Self::with_capacity(4)
    }

    pub fn with_capacity(size: usize) -> Self {
        Self {
            map: HashMap::with_capacity(size)
        }
    }
}
