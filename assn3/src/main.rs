use argh::FromArgs;
use core::panic;
use std::{
    fs::{read_dir, File},
    io::{BufWriter, Write},
    sync::{Arc, Mutex},
    thread,
};
use unix_semaphore::Semaphore;

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
    let output_file = File::create(args.output_file).expect("Error opening output file:");
    let writer = BufWriter::new(output_file);
    let file_mutex = Arc::new(Mutex::new(writer));

    // Setup input files
    let input_iter = read_dir(args.input_folder).expect("Error reading input directory:");

    // Semaphore to only let us spawn so many threads
    let sem = Arc::new(Semaphore::anonymous(10).unwrap());

    // Vec of all of our file paths to process
    let mut path_vec = vec![];

    // Iterate over every input file and add to the vec of file names
    for file in input_iter {
        let file = file.expect("Error reading input file:");
        if file.file_type().expect("Error reading file type:").is_file() {
            let path = file.path();
            if path.extension().expect("Error reading file extension:") != "png" {
                continue;
            };
            let path_str = path.to_str().expect("Error determining file path:").to_string();
            path_vec.push(path_str);
        }
    }

    // Vec of our thread handles
    let mut handles = vec![];

    // Note: If you wanted to do this 'properly', the _entire_ thread logic could be done via rayon w/
    // `path_list.par_iter().for_each(|path| process_image(path, out));`

    // Iterate over every path and spin off a thread if we can, else wait
    for path in path_vec.iter() {
        let infile = path.clone();
        let outfile = Arc::clone(&file_mutex);

        let sema = Arc::clone(&sem);

        sema.wait();

        let handle = thread::spawn(move || {
            println!("thread");
            process_image(outfile, infile, sema);
        });
        handles.push(handle);
    }

    // Sync up all thread handles and wait for them to finish.
    for handle in handles {
        handle.join().expect("Child thread has panicked:");
    }

    // Flush all buffered data to our output before exiting.
    file_mutex
        .lock()
        .expect("Error obtaining mutex lock:")
        .flush()
        .expect("Failed to flush data buffer:");

    println!("Exiting...");
}

fn process_image(mutex: Arc<Mutex<BufWriter<File>>>, path: String, sema: Arc<Semaphore>) {
    let mut writer = mutex.lock().expect("error obtaining mutex lock");

    let image = match lodepng::decode32_file(&path) {
        Ok(b) => b,
        Err(e) => {
            panic!("Error {} encountered reading image data for {}.", e, path);
        }
    };

    // Histogram for every R, G, and B.
    let mut hist = [0; (256 * 3)];

    for iter in image.buffer.iter() {
        let pixel = iter;

        let red: usize = pixel.r.into();
        let green: usize = pixel.g.into();
        let blue: usize = pixel.b.into();

        hist[red] += 1;
        hist[256 + green] += 1;
        hist[256 + 256 + blue] += 1;
    }

    for iter in hist.iter().enumerate() {
        let avg: f32 = *iter.1 as f32 / (image.width as f32 * image.height as f32 * 3.0);
        writeln!(writer, "{}:{}\t{}", iter.0, iter.1, avg).expect("Failed to write to output file:");
    }

    match sema.post() {
        Ok(_) => (),
        Err(o) => panic!("{}", o),
    };
}
