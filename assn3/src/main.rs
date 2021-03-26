use argh::FromArgs;
use std::{
    fs::File,
    io::{BufWriter, Write},
    sync::{Arc, Mutex},
    thread
};
use tokio::sync::Semaphore;

#[derive(Debug, FromArgs)]
/// Folder histogram generator
struct Arguments {
    /// number of threads to spawn
    #[argh(positional)]
    thread_count: usize,

    /// folder for input images
    #[argh(positional)]
    process_folder: String,

    /// output file name
    #[argh(positional)]
    output_file: String,
}

fn main() {
    let args: Arguments = argh::from_env();

    // Setup output file
    let output_file = File::create(args.output_file).expect("Error opening output file.");
    let writer = BufWriter::new(output_file); // Buffered writing
    let counter = Arc::new(Mutex::new(writer));

    // write!(output, "foo").expect("Failure writing");

    // Semaphore to only let us spawn so many threads
    let sem = Semaphore::new(args.thread_count);
    /*
        iterate over file names
        dec sema / spawn thread
        do work - need mutex on output file writing
        inc sema
    */
    let _ = sem.acquire();

    let mut handles = vec![];

    for _ in 0..=10 {
        let counter = Arc::clone(&counter);
        let handle = thread::spawn(move || {
            let mut mutex = counter.lock().expect("error obtaining mutex lock");

            mutex.write_all(b"foo").unwrap();
            mutex.flush().unwrap();
        });
        handles.push(handle);
    }
    for handle in handles {
        handle.join().unwrap();
    }

    //println!("Result: {}", *counter.lock().unwrap());

    println!("Exiting...");
}
