use std::fmt;
use enumflags2::BitFlags;
use crate::attribute::Attribute;

#[derive(Eq, PartialEq, Hash, Copy, Clone)]
pub struct StyleGroup {
    prefix: &'static str,
    assigner: Option<&'static str>,
    delimiter: Option<&'static str>,
    attribute: BitFlags<Attribute>,
}

impl StyleGroup {
    pub fn new(
        prefix: &'static str,
        assigner: Option<&'static str>,
        delimiter: Option<&'static str>,
        attribute: BitFlags<Attribute>,
    ) -> Self {
        Self {
            prefix,
            assigner,
            delimiter,
            attribute,
        }
    }

    pub fn prefix(&self) -> &'static str {
        self.prefix
    }

    pub fn assigner(&self) -> Option<&'static str> {
        self.assigner
    }

    pub fn delimiter(&self) -> Option<&'static str> {
        self.delimiter
    }

    pub fn attribute(&self) -> BitFlags<Attribute> {
        self.attribute
    }

    pub fn gnu_style() -> StyleGroup {
        StyleGroup {
            prefix: "-",
            assigner: Some("="),
            delimiter: Some(","),
            attribute: Attribute::gnu_style(),
        }
    }

    pub fn dos_style() -> StyleGroup {
        StyleGroup {
            prefix: "/",
            assigner: None,
            delimiter: Some(","),
            attribute: Attribute::dos_style(),
        }
    }
}

impl fmt::Debug for StyleGroup {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let StyleGroup {
            prefix,
            assigner,
            delimiter,
            attribute,
        } = self;

        if attribute.contains(Attribute::NoAssigner | Attribute::NoSparseAssign)
            && !attribute.contains(Attribute::CompactValue) {
            write!(f, "{prefix}<name>")?;
        }

        let value = if delimiter.is_none() {
            "value"
        } else {
            "value_1,value_2,value_N"
        };
        let assigner = if assigner.is_none() {
            ""
        } else {
            assigner.unwrap()
        };
        let space = if attribute.contains(Attribute::NoSparseAssign) {
            ""
        } else {
            " "
        };

        if attribute.contains(Attribute::CompactValue) {
            write!(
                f,
                "{prefix}<name>{space}[{assigner}{value}]",
                assigner = if attribute.contains(Attribute::NoAssigner) {
                    ""
                } else {
                    assigner
                }
            )?;
        } else {
            write!(
                f,
                "{prefix}<name>{space}[{assigner}{value}]",
                assigner = if attribute.contains(Attribute::NoAssigner) {
                    ""
                } else {
                    assigner
                }
            )?;
        }

        Ok(())
    }
}

pub struct Style {
    pub groups: Vec<StyleGroup>,
    pub terminators: Vec<&'static str>
}

impl Style {
    pub fn new() -> Style {
        Self::with_capacity(8, 2)
    }

    pub fn with_capacity(group_size: usize, terminator_size: usize) -> Style {
        Style {
            groups: Vec::with_capacity(group_size),
            terminators: Vec::with_capacity(terminator_size)
        }
    }

    pub fn group(&mut self, style_group: StyleGroup) -> usize {
        self.groups.push(style_group);
        self.groups.len() - 1
    }

    pub fn terminator(&mut self, terminator: &'static str) -> usize {
        self.terminators.push(terminator);
        self.terminators.len() - 1
    }
}
