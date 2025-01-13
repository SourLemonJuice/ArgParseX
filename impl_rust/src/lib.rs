pub struct Parser {

}

impl Parser {
    pub fn add_argument() {

    }
}

#[export_name = "hello"]
pub extern "C" fn hello() {
    println!("Hello World From Rust!")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        hello();
    }
}
