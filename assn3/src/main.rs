use argh::FromArgs;
use core::panic;
use std::{
    convert::TryInto,
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

    /// number of kilobytes for buffered writing
    #[argh(option, default = "4")]
    buffer: usize,
}

/// For our file data that we pass to our threads
struct FileData {
    path: String,
    name: String,
}

fn main() {
    let args: Arguments = argh::from_env();

    // Setup output file
    let output_file = File::create(args.output_file).expect("Error opening output file:");
    let writer = BufWriter::with_capacity(args.buffer * 1000, output_file); // bytes -> kilobytes
    let file_mutex = Arc::new(Mutex::new(writer));

    // Setup input files
    let input_iter = read_dir(args.input_folder).expect("Error reading input directory:");

    let thread_count: i32 = args.thread_count.try_into().unwrap();

    // Semaphore to only let us spawn so many threads
    let sem = Arc::new(Semaphore::anonymous(thread_count).unwrap());

    // Vec of all of our (file paths, filename) to process
    let mut file_vec = vec![];

    // Iterate over every input file and add to the vec of file names
    for file in input_iter {
        let file = file.expect("Error reading input file:");
        if file.file_type().expect("Error reading file type:").is_file() {
            let path = file.path();
            if path.extension().expect("Error reading file extension:") != "png" {
                continue;
            }

            let filename: String = path
                .file_name()
                .expect("Error determining file name:")
                .to_owned()
                .into_string()
                .expect("Faulty unicode in file name:");

            let path_str = path.to_str().expect("Error determining file path:").to_string();
            let data = FileData {
                path: path_str,
                name: filename.clone(),
            };
            file_vec.push(Arc::new(data));
        }
    }

    // Vec of our thread handles
    let mut handles = vec![];

    // Note: If you wanted to do this 'properly', the _entire_ thread logic could be done via rayon w/
    // `path_list.par_iter().for_each(|path| process_image(path, out));`

    // Iterate over every path and spin off a thread if we can, else wait
    for data in file_vec.iter() {
        let data = data.clone();
        let outfile = Arc::clone(&file_mutex);

        // Get a local instance of the sema to pass and wait on it
        let sema = Arc::clone(&sem);
        sema.wait();

        // Spawn our thread since we're not blocked
        let handle = thread::spawn(move || {
            process_image(outfile, data, sema);
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

fn process_image(mutex: Arc<Mutex<BufWriter<File>>>, fdat: Arc<FileData>, sema: Arc<Semaphore>) {
    let mut writer = mutex.lock().expect("error obtaining mutex lock");

    let image = match lodepng::decode32_file(&fdat.path) {
        Ok(b) => b,
        Err(e) => {
            panic!("Error {} encountered reading image data for {}.", e, fdat.path);
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

    for iter in hist.iter() {
        let freq: f32 = *iter as f32 / (image.width as f32 * image.height as f32);
        write!(writer, "{} ", freq).expect("Failed to write to output file:");
    }

    writeln!(writer, "# {}", fdat.name).expect("Failed to write to output file:");

    sema.post().unwrap();
}
