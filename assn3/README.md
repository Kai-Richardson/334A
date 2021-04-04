# Folder-Histogram
### By Kai Richardson

To install Rust:
Run `install-rust.sh`. Follow the defaults (stable, likely x64).

To compile: `cargo build --release`

To run:
There's two ways you can do this,
1. Run from cargo directly via something like: `cargo run --release 16 sampleImagesBig/ output.txt`
2. Run from the executable, essentially go to `target/release` and run the executable as normal after running `cargo build --release` to compile the program.

If you want to watch it run in ~slow motion~ (so you can see the threads in htop), just don't add the release flag. It'll be much _much_ slower.

I also added an option (`--buffer X`, must go after positional arguments) to change the number of kilobytes for the buffered writing memory.

Note:
This is currently busted on the main Virtual Desktop, due to permissions for cargo/rustc being really really messed up.
They prevent me from bootstraping my own installation at the present.

The arguments don't show any help text for the positional arguments, working on a fix for the crate.
