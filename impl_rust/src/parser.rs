use crate::flag::{Flag, Flags};
use crate::results::Error;
use crate::style::Style;

pub struct Parsed {
    pub args: Vec<String>
}

pub struct Parser<'a> {
    flags: Flags<'a>,
    style: Style
}

impl<'a> Parser<'a> {
    pub fn new(style: Style) -> Parser<'a> {
        Self {
            style,
            flags: Flags::new()
        }
    }

    pub fn flag(&mut self, flag: Flag<'a>) {
        assert!(self.style.groups.len() >= flag.group_id, "invalid group_id in flag");
        self.flags.map.entry(self.style.groups[flag.group_id])
            .or_default()
            .push(flag);
    }

    pub fn parse(&self, args: Vec<String>, terminates_at: usize) -> Result<Parsed, Error> {
        let mut params = Vec::with_capacity(args.len());

        let iter = args
            .into_iter()
            .peekable();

        let groups: &Vec<_> = &self.style.groups;


        for arg in iter {
            let group = match groups.iter().find(|group| arg.starts_with(group.prefix())) {
                Some(group) => group,
                None => {
                    params.push(arg);
                    continue;
                }
            };
            let part = &arg[group.prefix().len()..];
            println!("parsed: {}", part)
        }

        println!("parse params: {:?}", params);

        Ok(Parsed {
            args: params
        })
    }
}
