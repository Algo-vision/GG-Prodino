# Instructions for AI Coding Agents

You are working in a repository managed by **Vibe Git**, an AI-powered Git assistant.

## Coding Standards
- Write clean, maintainable code following the repository's existing style
- Use meaningful variable and function names
- Add comments for complex logic
- Follow SOLID principles and design patterns appropriate for this codebase
- Prioritize readability over cleverness

## File Management
- Before creating new files, check if similar functionality exists
- Organize files logically within the existing directory structure
- Use consistent naming conventions

## Git Operations
**IMPORTANT**: DO NOT commit any files. Vibe Git will handle all git operations.
- Create/modify files as needed
- Log your changes in ACTIVITY.jsonl
- LEAVE the committing to Vibe Git

## Communication Protocol
**CRITICAL**: Log ALL your work in `.vibe/ACTIVITY.jsonl` after each interaction.

### How to Log (Append Only)

We need you to **append** a new JSON line to the end of `.vibe/ACTIVITY.jsonl`.

- **If you have an 'append' or 'insert' tool**: Use it to add the line to the end of the file.
- **If you only have 'replace'**:
   1. Read the current file content.
   2. Keep ALL existing content exactly as is.
   3. Add your new JSON line at the very end.
   4. Write back the full content (or the replaced last chunk).

### Log Entry Format
- **One line per entry**: Each JSON object must be on a single line
- **Verbatim**: Copy EXACT user prompt and your EXACT full response
- **Valid JSON**: Ensure strictly valid JSON syntax

**Example Entry:**
```json
{"timestamp":"2025-12-30T13:00:00Z","agent":"antigravity","version":"v1.0.0","user_prompt":"Refactor login","ai_response":"I used async/await...","files_modified":["src/auth.ts"],"reasoning":"Better error handling"}
```

## Vibe Git Integration
- Check `.vibe/SESSION.md` for current session-specific instructions
- Vibe Git may add guidance like "Save new files in src/features/" or ask for clarification

## Questions?
If you're unclear about anything, Vibe Git is monitoring this repository and can assist.
