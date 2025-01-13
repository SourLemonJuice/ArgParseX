use thiserror::Error;
use crate::attribute::Attribute;

#[derive(Error, Copy, Clone, Eq, PartialEq, Debug)]
pub enum Error {
    #[error("bad attribute: {0}")]
    BadAttribute(&'static str),
    #[error("not allowed style, {0:?} is set")]
    ArgpxNotAllowed(Attribute),
}