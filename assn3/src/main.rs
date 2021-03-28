use argh::FromArgs;
use std::{
    fs::{read_dir, File},
    io::{BufWriter, Write},
    sync::{Arc, Mutex},
    thread,
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
    input_folder: String,

    /// output file name
    #[argh(positional)]
    output_file: String,
}

fn main() {
    let args: Arguments = argh::from_env();

    // Setup output file
    let output_file = File::create(args.output_file).expect("Error opening output file.");
    let writer = BufWriter::new(output_file);
    let counter = Arc::new(Mutex::new(writer));

    // Setup input files
    let input_iter = read_dir(args.input_folder).expect("Error reading input directory.");

    // Semaphore to only let us spawn so many threads
    // let sem = Semaphore::new(args.thread_count);

    // Iterate over every input file
    for file in input_iter {
        let file = file.expect("Error reading input file");
        let path = file.path().to_str().expect("Error determining file path").to_string();
        let image = match lodepng::decode32_file(path.clone()) {
            Ok(b) => b,
            Err(e) => {
                print!("Error {} encountered reading image data for {}.", e, path);
                continue;
            }
        };
        println!("{}", image.height);
    }

    /*
        // iterate over file names
        dec sema / spawn thread
        do work - need mutex on output file writing
        inc sema
    */
    //let _ = sem.acquire();

    // Vec of our thread handles
    let mut handles = vec![];

    for _ in 0..args.thread_count {
        let counter = Arc::clone(&counter);
        let handle = thread::spawn(move || {
            let mut mutex = counter.lock().expect("error obtaining mutex lock");

            write!(mutex, "foo\n").expect("failed to write to output file");
        });
        handles.push(handle);
    }

    // Sync up all thread handles and wait for them to finish.
    for handle in handles {
        handle.join().unwrap();
    }

    // Be sure to flush all buffered data to our output before exiting.
    counter
        .lock()
        .expect("error obtaining mutex lock")
        .flush()
        .expect("failed to flush data buffer");

    println!("Exiting...");
}
