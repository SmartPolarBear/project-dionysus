## Project "Dionysus"  
The project "Dionysus" aims at applying theories and practices concerning operation systems to a hybrid kernel in modern C++. In the long run, It aims at creating an effective microkernel with acceptable performance overhead.  

[![issues](https://img.shields.io/github/issues/SmartPolarBear/project-dionysus)](https://github.com/SmartPolarBear/project-dionysus/issues)
[![forks](https://img.shields.io/github/forks/SmartPolarBear/project-dionysus)](https://github.com/SmartPolarBear/project-dionysus/fork)
[![stars](https://img.shields.io/github/stars/SmartPolarBear/project-dionysus)](https://github.com/SmartPolarBear/project-dionysus/stargazers)
[![license](https://img.shields.io/github/license/SmartPolarBear/project-dionysus)](https://github.com/SmartPolarBear/project-dionysus/blob/master/LICENSE)
[![twitter](https://img.shields.io/twitter/url?style=social&url=https%3A%2F%2Ftwitter.com%2F___zirconium___)](https://twitter.com/___zirconium___)


### Motivation  

The techniques used and concepts of programming and design of operation system have changed over time. That's why I want to adapt the advanced and exciting ones in a project as an attempt to do further researches in this field.  

### Features and goals  

- Multi-server microkernel approach  
- Asynchronous OS API  
- Written in modern C++  

### Built With  

#### Environment  

The project is initially built with  

- clang/ LLVM 13  
- CMAKE, 3.19 and above.  

on Debian in WSL2.  

**Note: ** Other compilers may be competent, but C++20 support for compiler is required, for C++20 features like `std::span` are massively used.  

#### Third-party  

- [newlib-cygwin](https://sourceware.org/git/gitweb.cgi?p=newlib-cygwin.git)  
- [microsoft/GSL](https://github.com/microsoft/GSL.git)  
- [llvm/llvm-project](https://github.com/llvm/llvm-project)    


## Roadmap  

| Q3 2021 | Q1 2022 | Far Future |
|---------|---------|-----------|
✅ Basic OS structure <br> 🔄Kernel-mode servers <br> 🔄 Basic userland and shell | ❌ Extended OS features <br> ❌ OS security features <br> ❌ Asynchronous API <br> ❌ Graphic support  |  ❌ Window system ❌ Accessories  


✅ Supported | 🔄 In progress | ❌ In plan  


## Contributing

Contributions are what make the open source community such an amazing place to be learned, inspire, and create. Any contributions you make are **greatly appreciated**.  

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)  
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)  
4. Push to the Branch (`git push origin feature/AmazingFeature`)  
5. Open a Pull Request  

Furthermore, you are welcomed to:  

1. [Ask a question](https://github.com/SmartPolarBear/project-dionysus/discussions/categories/q-a)   
   Also, have a look at our [FAQs]().  
2. [Start a discussion](https://github.com/SmartPolarBear/project-dionysus/discussions/categories/general)    
   Discussions can be about any topics or ideas related to CLOX.  
3. [Make a feature proposal](https://github.com/SmartPolarBear/project-dionysus/issues)   
   Language features do you want to appear or not to appear in CLOX? For example, you can propose a new grammar making the lox better, or an idea for library features.   

## Credits
The development of this project mostly refers to, and sometimes borrows some techniques, designs and codes from the following project:  
- [mit-pdos/xv6-public](https://github.com/mit-pdos/xv6-public)  
xv6 inspired me to start the project to build a kernel, with which I got knowledge of operation systems. And some of its shortcomings motivated me to create this project. 
- [Stichting-MINIX-Research-Foundation/minix](https://github.com/Stichting-MINIX-Research-Foundation/minix)    
A series of papers and the book about the development of minix 3 not only let me know about what is microkernel and how it works but also have a deep influence on the goal and development of this project.

- [vsrinivas/fuchsia](https://github.com/vsrinivas/fuchsia)  
Google's new operation system.  

## License
Copyright (c) 2021 SmartPolarBear

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.