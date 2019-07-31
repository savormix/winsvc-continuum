# Purpose
An executable that changes the startup type of a Windows service to "Automatic" and the status to "Running".

# Usage
`winsvc_continuum.exe --service service --interval interval`
- **service:** service name (note: not the display name)
- **interval:** how often to examine/change service configuration and status

## Implementation details
Application will query the service configuration for the current startup type to determine if it needs to be changed.

The same applies to the service state. It will not start the service if it is:
- Running
- Starting
- Stopping
- Pausing
- Resuming

No action is taken during transitional statuses due to the fact that this application is intended to be executed at
regular intervals (or once with `--interval` set).
