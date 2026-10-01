mod artwork;
mod signature;
mod validate;

use std::{env, error::Error, path::Path};
type Result<T = ()> = std::result::Result<T, Box<dyn Error>>;

fn main() {
    if let Err(error) = run() {
        eprintln!("Error: {error}");
        std::process::exit(1);
    }
}

fn run() -> Result {
    let root = Path::new(env!("CARGO_MANIFEST_DIR"));
    match env::args().nth(1).as_deref() {
        Some("generate-artwork") => artwork::generate(root),
        Some("validate") => validate::validate(root),
        Some("test-bootstrap-signature") => signature::test(),
        _ => Err(
            "Usage: cargo run --locked -- <generate-artwork|validate|test-bootstrap-signature>"
                .into(),
        ),
    }
}
