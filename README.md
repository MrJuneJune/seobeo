# SeoBeo

<p align="center">
  <img src="/seobeo.png" width="350" height="350" alt="SeoBeo Logo"/>
</p>

> ⚠️ **Warning**: SeoBeo is a work in progress. It’s not production-ready—use at your own risk.

SeoBeo is a lightweight HTTP server written in C, designed for educational use and personal projects. I built this after getting tired of bloated or constantly changing API libraries. Writing a server from scratch in C makes it easier to reason about the entire stack and truly understand how things work under the hood.

## 🚀 Getting Started

```bash
make example
```

This will compile and run the example server with basic routes and static file handling.

## ✅ Features

* Epoll-based edge-triggered event loop
* Minimalist HTTP routing
* Static asset serving
* File-based response support
* Example integration with a PostgreSQL database
  - Using [PogPool](https://github.com/MrJuneJune/pog_pool)

## 🔜 Roadmap

* CSRF protection for form submissions
* Server-side rendering with simple MVP-style templates
* WebSocket support
* Server-Sent Events (SSE)
* Docker support
