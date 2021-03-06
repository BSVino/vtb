vtb
===

Vino's Tool Box -- stb-style public domain libraries

This code is in the public domain. No warranty is offered or implied. Use this code at your own risk. "Public domain" means that I make no legal claim to this code, and it is under no copyright, and you can do anything you want to do with it.

library              | category | description
-------------------- | -------- | --------------------------------
**vtb.h**            | misc     | Helper utilities and preproc defines commonly used in large projects
**vtb_alloc_ring.h** | memory   | A no-copy variable-allocation-size contiguous-memory ring allocator
**vtb_hash.h**       | utility  | A fast hash function for hash tables and integrity checking

The inspiration for these libraries is the [stb libraries](https://github.com/nothings/stb). Sean Barrett, who wrote the stb libraries, delivered a talk on why code reuse is important, which you can see here: https://www.youtube.com/watch?v=eAhWIO1Ra6M That talk was the primary motivation for starting my own libraries.

I welcome any contributions to vtb, so long as you agree that your contributions will also be placed in the public domain.