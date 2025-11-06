# 🧩 JSON Configuration Loader with Recursive Includes

A lightweight, header-only **C++17 JSON configuration system** that supports recursive `include` directives.  
Built on top of the excellent [nlohmann/json](https://github.com/nlohmann/json) library.

This loader lets you split your configuration into smaller, modular files — automatically nesting or merging them into a single resolved structure.  
It’s ideal for **game engines**, **renderers**, or any tool needing structured, layered configuration.

---

## ✨ Features

- 📂 **Recursive includes**
  ```json
  { "include": "renderer.json" }
