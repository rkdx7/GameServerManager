#include "RemoteShell.h"
#include <QProcess>

namespace RemoteShell {

QString shQuote(const QString &s)
{
    QString e = s;
    e.replace("'", "'\\''");
    return "'" + e + "'";
}

QStringList sshBaseArgs(const VMInstance &vm)
{
    QStringList a;
    a << "-o" << "StrictHostKeyChecking=no"
      << "-o" << "BatchMode=yes"
      << "-p" << QString::number(vm.sshPort);
    if (!vm.sshKeyPath.isEmpty())
        a << "-i" << vm.sshKeyPath;
    a << QStringLiteral("%1@%2").arg(vm.sshUser, vm.host);
    return a;
}

QString sudoPrefix(const VMInstance &vm)
{
    if (vm.isRoot())
        return QString();
    if (!vm.sudoPassword.isEmpty())
        return "echo " + shQuote(vm.sudoPassword) + " | sudo -S -p '' ";
    return "sudo ";
}

QString privileged(const VMInstance &vm, const QString &script)
{
    QString inner = "bash -c " + shQuote(script);
    if (vm.isRoot())
        return inner;
    if (!vm.sudoPassword.isEmpty())
        return "echo " + shQuote(vm.sudoPassword) + " | sudo -S -p '' " + inner;
    return "sudo " + inner;
}

QString run(const VMInstance &vm, const QString &command, int timeoutMs)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    QStringList args = sshBaseArgs(vm);
    args << command;
    proc.start("ssh", args);
    proc.waitForFinished(timeoutMs);
    return QString::fromUtf8(proc.readAll());
}

QString detectOsId(const VMInstance &vm)
{
    QString out = run(vm, ". /etc/os-release 2>/dev/null && echo \"$ID $ID_LIKE\"", 15000);
    return out.trimmed().toLower();
}

QString interactiveSudoCommand(const VMInstance &vm, const QString &script)
{
    QString cmd = "ssh -t -o StrictHostKeyChecking=no -p " + QString::number(vm.sshPort);
    if (!vm.sshKeyPath.isEmpty())
        cmd += " -i \"" + vm.sshKeyPath + "\"";
    cmd += " " + vm.sshUser + "@" + vm.host;
    // Remote command: sudo bash -c '<script>' (single-quote the script for the
    // remote shell), wrapped in double quotes for the local shell.
    QString remote = "sudo bash -c '" + QString(script).replace("'", "'\\''") + "'";
    cmd += " \"" + remote.replace("\"", "\\\"") + "\"";
    return cmd;
}

bool launchInTerminal(const QString &command)
{
    // `cmd /k` keeps the window open after ssh exits so the user can read output.
    return QProcess::startDetached("cmd", {"/c", "start", "GameServerManager — Docker",
                                           "cmd", "/k", command});
}

} // namespace RemoteShell
