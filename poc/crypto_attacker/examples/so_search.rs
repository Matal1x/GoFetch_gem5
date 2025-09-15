use crypto_attacker::NATIVE_PAGE_SIZE;
// std lib
use std::process::id;
use std::str;
// file io
use std::fs::{self, File};
use std::io::Write;
use std::env;
// regex for optional filtering
use regex::Regex;

fn main() {
    // optional: allow overriding output filename via first CLI arg
    let args: Vec<String> = env::args().collect();
    let out_filename = if args.len() > 1 {
        args[1].clone()
    } else {
        String::from("so_space.txt")
    };

    // Read /proc/<pid>/maps of current process
    let pid = id();
    let maps_path = format!("/proc/{}/maps", pid);
    let maps_content = fs::read_to_string(&maps_path)
        .expect(&format!("Failed to read {}", maps_path));
    let vmmap_lines = maps_content.lines();

    // Variables to store start/end of first large .so region
    let mut first_flag = false;
    let mut start_addr: u64 = 0;
    let mut end_addr: u64 = 0;

    // Iterate over all mapped regions
    for line in vmmap_lines {
        // Linux heuristic: shared objects / dynamic loader entries
        if line.contains(".so") || line.contains("ld-") || line.contains("ld.so") {
            // Parse the first two hex addresses in the line (start-end)
            let mut addr_tokens: Vec<u64> = vec![];
            for token in Regex::new(r"[0-9a-f]{6,16}").unwrap().find_iter(line) {
                let val = u64::from_str_radix(token.as_str(), 16)
                    .expect("Failed to parse address from /proc/<pid>/maps");
                addr_tokens.push(val);
            }

            // Need at least start and end addresses
            if addr_tokens.len() < 2 {
                continue;
            }

            if !first_flag {
                start_addr = addr_tokens[0];
                end_addr = addr_tokens[1];
                first_flag = true;
            } else {
                // Extend contiguous region if possible
                if end_addr == addr_tokens[0] {
                    end_addr = addr_tokens[1];
                } else {
                    start_addr = addr_tokens[0];
                    end_addr = addr_tokens[1];
                }
            }

            // Stop once we cover >=200 pages
            if (end_addr - start_addr) / NATIVE_PAGE_SIZE as u64 >= 200 {
                break;
            }
        }
    }

    // Panic if no .so region was found
    if !first_flag {
        panic!("[!] Fail to get start addr / end addr for target pointer! No .so regions found.");
    }

    // Write the start/end addresses to output file
    let mut out_file = File::create(&out_filename)
        .expect(&format!("Failed to create {}", &out_filename));
    write!(out_file, "{:#x}\n", start_addr).unwrap();
    write!(out_file, "{:#x}\n", end_addr).unwrap();

    println!("[+] Target Pointer start frame -> {:#x}", start_addr);
    println!("[+] Target Pointer end frame   -> {:#x}", end_addr);
    println!("[+] Wrote {}.", out_filename);
}
