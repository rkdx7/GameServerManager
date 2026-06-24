#pragma once
#include "VMInstance.h"
#include <QString>
#include <QStringList>

// Small, stateless helpers for running commands on a VM over SSH. Centralises the
// ssh argument building (previously duplicated across DockerManager / VMAdminPage)
// and adds sudo handling for non-root accounts.
namespace RemoteShell {

// Single-quote a string for safe use inside a POSIX shell command.
QString shQuote(const QString &s);

// Base `ssh` CLI arguments (options + user@host), without the trailing command.
QStringList sshBaseArgs(const VMInstance &vm);

// Prefix to elevate a single command:
//   root            -> ""                            (no elevation)
//   non-root + pass -> "echo '<pass>' | sudo -S -p '' "
//   non-root, no pw -> "sudo "
QString sudoPrefix(const VMInstance &vm);

// Wrap a (possibly multi-command) shell script so it runs with the privileges
// required by the VM's user — the whole script runs under `bash -c`, elevated
// with sudo when the user is not root.
QString privileged(const VMInstance &vm, const QString &script);

// Run a command on the VM over SSH (blocking — call from a worker thread).
// Returns stdout and stderr merged.
QString run(const VMInstance &vm, const QString &command, int timeoutMs = 30000);

// Whitespace-separated lowercase OS identifiers from the VM's /etc/os-release
// ($ID followed by $ID_LIKE), e.g. "ubuntu debian". Empty on failure.
QString detectOsId(const VMInstance &vm);

// Ready-to-run `ssh -t … sudo bash -c '<script>'` command that allocates a TTY so
// the remote sudo can prompt for a password interactively. Used as the manual
// fallback when no sudo password is stored: the user only has to type their
// password once the command runs.
QString interactiveSudoCommand(const VMInstance &vm, const QString &script);

// Best-effort launch of `interactiveSudoCommand` in a new external terminal
// window (Windows `cmd /k`). Returns false if the terminal could not be started.
bool launchInTerminal(const QString &command);

} // namespace RemoteShell
