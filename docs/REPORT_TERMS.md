# Reading the diagnostic reports

- **Hardware-crossed**: the observed call completed on Switch and execution reached
  a later attributable operation. A hit counter alone does not establish this.
- **Frontier**: the first current unsupported call or failure that stops the run.
- **First-hit**: the saved arguments from the first occurrence of a target in a run;
  later occurrences can have different arguments.
- **Scoped providers**: the implementation definitions checked for a specified set
  of linked symbols. This is not a claim that every symbol in the binary was audited.
- **Present**: a request to display a rendered surface. Its success counter says
  nothing about whether the image contains correct game pixels.

Current results distinguish public synthetic tests, private rendered builds,
observed console progression and actual image evidence. See the
[roadmap](../ROADMAP.md) and [surface-image diagnostic](SWITCH_FRAME_DUMP.md).
