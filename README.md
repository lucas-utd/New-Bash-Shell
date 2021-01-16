<!-- ABOUT THE PROJECT -->

## About The Project

It is a simple shell as a project for the course Operating Systems.
The goal of the project is to let you get familiar with some process related system calls in Unix (or Linux) systems, including how to create processes and how to communicate between processes. The projects
have to be programmed in C or C++ and executed under Unix (or Linux and alike).

[![Product Name Screen Shot][home-page]]

### Realized functions

- pipe, which can executes a sequence of shell command separated by ";"
- multi-user, many users can logins shell
- socket commnuication, users use sockets communicate with main shell
- Interrupt Handling, admin uses SIGQUIT signal to kill all user processes

## Source Files Functions

- The file UM.c realizes user manager.
- The file shsh.c realizes the shsh shell.
- The file user.c realizes the user.
- The file admin.c realizes the admin.
- The DesignDoc file, which contains the pseudo-code for UM and Shsh and the description of the major features of the program.

## Built With

The Makefile that generates the executables from source code files. The Makefile generate 3 executables "UM.exe", "user.exe", "admin.exe".

- [gcc](https://gcc.gun.org)

<!-- GETTING STARTED -->

## Getting Started

### Enter bash shell

```
$ make

```

<!-- CONTRIBUTING -->

## Contributing

Contributions are what make the open source community such an amazing place to be learn, inspire, and create. Any contributions you make are **greatly appreciated**.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<!-- LICENSE -->

## License

Distributed under the MIT License. See `LICENSE` for more information.

<!-- CONTACT -->

## Contact

- LinkedIn: [linkedin](https://linkedin.com/in/tao-chen-lucas)

- Project Link: [https://github.com/lucas-utd/Bicycles-Shop-Final](https://github.com/lucas-utd/Bicycles-Shop-Final)

<!-- ACKNOWLEDGEMENTS -->

<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->

[linkedin]: https://linkedin.com/in/tao-chen-lucas
[home-page]: images-readme/home-page.png
[cart-page]: images-readme/cart-page.png
[order-list-page]: images-readme/order-list-page.png
[product-list-page]: images-readme/product-list-page.png
[search-page]: images-readme/search-page.png
[user-update-profile-page]: images-readme/user-update-profile-page.png
