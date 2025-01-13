use enumflags2::{bitflags, BitFlags};

#[bitflags]
#[repr(u8)]
#[derive(Eq, PartialEq, Copy, Clone, Debug)]
pub enum Attribute {
    /// schema: -a -b -c
    /// input: -abc
    /// output: Ok<{ a: true, b: true, c: true }>
    Composable,
    /// schema: -a: number
    /// input: -a8080
    /// output: Ok<{ a: 8080 }>
    CompactValue,
    /// schema: /a: string /b: number
    /// input: /a=127.0.0.1 /b=8080
    /// output: Ok<{ a: '127.0.0.1', b: '8080' }>
    CompactArg,
    /// schema: -a: number
    /// input: -a=8080
    /// output: Err<ArgpxNotAllowed<Attribute::NoAssigner>>
    NoAssigner,
    /// schema: -a: number
    /// input: -a 8080
    /// output: Err<ArgpxNotAllowed<Attribute::NoSparseAssign>
    NoSparseAssign,
}

impl Attribute {
    pub fn gnu_style() -> BitFlags<Self> {
        Attribute::Composable | Attribute::CompactValue
    }

    pub fn dos_style() -> BitFlags<Self> {
        Attribute::Composable | Attribute::CompactArg
    }

    pub fn default() -> BitFlags<Self> {
        Self::gnu_style()
    }
}
