# AI Coding Instructions

## Code Quality & Style
- Write clean, readable code with meaningful variable and function names
- Follow established coding standards and language conventions for spacing, comments, and naming
- Prioritize clarity to make reading, understanding, and modifying code easier
- Regularly review and refactor code to improve structure and maintainability
- Always leave the codebase cleaner than you found it

## Function Design Principles
- Write short functions that do one thing well (Single Responsibility Principle)
- If a function becomes too long or complex, break it into smaller, manageable functions
- Encapsulate nested if/else statements into functions with descriptive names
- Choose names that reflect purpose and behavior - if a name requires a comment, the name doesn't reveal its intent

## Comment Guidelines
- Use comments sparingly and make them meaningful when you do
- Focus on explaining the "why" behind decisions, not the "what"
- Don't comment on obvious things - excessive comments clutter the codebase
- Explain unusual behavior, potential pitfalls, and complex logic

## DRY Principle & Code Reuse
- Avoid duplicating code or logic (Don't Repeat Yourself)
- Reuse code through functions, classes, modules, or other abstractions
- Design changes to require modification in only one place

## Embedded Systems Best Practices
- Always validate input parameters and array bounds to prevent crashes
- Use appropriate data types (avoid unnecessary `int` when `uint8_t` suffices)
- Be mindful of memory usage - prefer stack allocation over dynamic when possible
- Consider real-time constraints - avoid blocking operations in main loops
- Initialize variables and arrays explicitly, especially for hardware state

## Arduino/PlatformIO Development
- Use `platformio.ini` for dependency management rather than manual library installation
- Test builds frequently with `pio run` before making large changes
- When adding new libraries, verify compatibility with the target platform
- Use Serial output for debugging, but remove or guard debug prints in production code

## Hardware Integration Patterns
- Separate hardware abstraction from business logic
- Use configuration files/headers to make hardware changes easy
- Implement graceful degradation when optional hardware is missing
- Always consider signal timing, debouncing, and electrical characteristics
- Document pin assignments and hardware requirements clearly

## Multi-Platform Considerations
- Keep platform-specific code isolated in dedicated files or sections
- Use preprocessor directives to handle platform differences cleanly
- Maintain consistent APIs across different hardware platforms
- Test configuration changes on all supported platforms when possible

## Git & Collaboration
- Make atomic commits with clear, descriptive messages
- Test builds before committing to avoid breaking the build
- Consider impact on other developers and platforms when making changes
- Keep branch-specific features isolated to avoid merge conflicts
- Use version control effectively to track changes and collaborate
