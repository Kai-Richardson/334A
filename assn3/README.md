To compile:
`cargo build --release`

To run:
There's two ways you can do this,
1. Run from cargo directly via something like: `cargo run --release 16 sampleImagesBig/ output.txt`
2. Run from the executable, essentially go to `target/release` and run the executable as normal after running `cargo build --release` to compile the program.

If you want to watch it run in ~slow motion~, just don't add the release flag. It'll be much much slower.

Note:
This is currently busted on the main Virtual Desktop, due to permissions for cargo/rust being really really messed up. They prevent me from bootstraping my own installation at the present.
