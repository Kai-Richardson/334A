use argh::FromArgs;
use tokio::spawn;
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

#[tokio::main]
async fn main() {
    let args: Arguments = argh::from_env();

    // if let Err(output_file) = File::open(args.output_file) {
    //     panic!("Error encountered opening the output file. Exiting...");
    // }
    // Semaphore to only let us spawn so many threads

    println!("Hello, world!");

    // iterate over file names
    // dec sema / spawn thread
    // do work - need mutex on output file writing
    // inc sema

    let sem = Semaphore::new(args.thread_count);

    let _ = sem.acquire().await;

    for _ in 0..10 {
        let task = spawn(write_to_output());
        task.await.unwrap();
    }

    println!("Exiting...");
}

async fn write_to_output() {
    println!("Foo");
    tokio::time::sleep(tokio::time::Duration::from_secs(1)).await;
    println!("Async task done.");
}
