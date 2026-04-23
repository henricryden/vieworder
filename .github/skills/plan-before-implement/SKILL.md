---
name: plan-before-implement
description: "Use when the user asks to implement a feature, add functionality, or make a non-trivial change. ALWAYS generate a markdown plan file first, ask the user to review it, then implement only after approval. Trigger phrases: 'implement', 'add feature', 'create', 'build', 'develop', 'make X work', 'set up'. Do not do this for trivial fixes or typos. If the user says 'just do it' or 'skip the plan', proceed directly to implementation."
argument-hint: "Description of the feature or task to plan"
---

# Plan Before Implement

## When to Use

Apply this skill whenever the user requests implementation of a feature, new functionality, or any non-trivial change. Do **not** write code until the plan has been reviewed and the user says to proceed.

## Procedure

### Step 1: Generate the Plan File

Create a markdown file describing the implementation plan. Choose a location that fits the project (e.g., a `plans/` folder in the workspace root, or another logical location). Name the file after the feature (e.g., `plans/add-dark-mode.md`).

The plan file must include:

```markdown
# Plan: <Feature Name>

## Goal
One-sentence description of what this implements and why.

## Approach
High-level strategy. Which files/modules are involved. Any key design decisions.

## Steps
1. <Concrete step — what file/function changes and why>
2. ...

## Out of Scope
Anything explicitly excluded from this implementation to keep it focused.

## Open Questions
Any ambiguities or decisions the user should weigh in on before implementation begins.
```

### Step 2: Ask the User to Review

After creating the plan file, tell the user:
- Where the plan file was saved
- To read through it and make any edits or comments directly in the file
- To reply (e.g., "looks good", "implement it", or with specific feedback) when ready

Do **not** write any implementation code at this stage.

### Step 3: Implement

Only after the user explicitly says to proceed (or approves the plan), implement exactly what is described in the plan file. Follow the steps in order. Do not add extra features or changes beyond the plan.

## Notes

- If the user's request is a single-line fix, a typo correction, or clearly trivial, use judgment about whether a full plan is warranted.
- If the user says "just do it" or "skip the plan", proceed directly to implementation.
- Keep plans concise — a plan is a guide, not a specification document.
