## Project "Dionysus"
The project "Dionysus" aims at applying theories and practices concerning operation systems to a hybrid kernel in modern C++. In the long run, It aims at creating an effective microkernel with acceptable performence overhead.

## Motivation

The techniques used and concepts of programming and design of operation system have changed over time. That's why I want to adapt the advanced and exciting ones in a project as an attempt to do further researches in this field.

## Technologies
  
- [Guidelines Support Library](https://github.com/microsoft/GSL.git)

- Limited usage of STL

- CMake, 3.16 or newer

- Clang/LLVM Toolchain


## Features

- Modern C++ (C++20)

- Aimed at Microkernel


## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.  

Please make sure to update tests as appropriate. 

## Credits
The project mostly refers to, and sometimes borrows some techniques, designs and codes from the following project:  
- [mit-pdos/xv6-public](https://github.com/mit-pdos/xv6-public)  
xv6 inspired me to start the project to build a kernel, with which I got knowledge of operation systems. And some of its shortcomings motivated me to create this project. 
- [Stichting-MINIX-Research-Foundation/minix](https://github.com/Stichting-MINIX-Research-Foundation/minix)    
A series of papers and the book about the development of minix 3 not only let me know about what is microkernel and how it works but also have a deep influence on the goal and development of this project.

- [vsrinivas/fuchsia](https://github.com/vsrinivas/fuchsia)  
Google's new operation system.  

## License
Copyright (c) 2021 SmartPolarBear

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.