# Contributing to BlueFare (MultiMic)

Thank you for your interest in contributing to **BlueFare MultiMic**!

## Code Style & Standards
- **C++ Code**: Written in **C++20**. Follow modern RAII principles, memory safety, and thread-safety guidelines. Avoid raw blocking calls on realtime audio threads.
- **Android Code**: Standard Java (Java 17 compatibility). Enforce strict Android permission handling, foreground service compliance, and UI feedback.
- **Protocol Changes**: Any modifications to TCP or UDP packet layouts must be documented in [protocol.md](file:///home/sabbir/Documents/BlueFare/protocol/protocol.md) and updated in both C++ and Java binary serializers.

## Submitting Pull Requests
1. Fork the repository on GitHub.
2. Create a feature branch (`git checkout -b feature/my-feature`).
3. Make your changes and add test coverage under `server/tests/`.
4. Ensure all CTest unit tests pass:
   ```bash
   mkdir -p build && cd build && cmake ../server && make && ctest --output-on-failure
   ```
5. Commit your changes and open a Pull Request.
