#include "StorageLinuxMountHelper.h"

#include <QFile>
#include <QTextStream>
#include <QStringList>

QString LinuxMountHelper::unescapeMountInfo(const QString& str) {
    QString res = str;
    int idx = 0;
    while ((idx = res.indexOf('\\', idx)) != -1) {
        if (idx + 3 < res.length()) {
            bool ok;
            int asciiVal = res.mid(idx + 1, 3).toInt(&ok, 8);
            if (ok) res.replace(idx, 4, QChar(asciiVal));
            else idx++;
        } else { idx++; }
    }
    return res;
}

bool LinuxMountHelper::getMountInfoForPath(const QString& targetPath, MountInfo& outInfo) {
    QFile file("/proc/self/mountinfo");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    int maxMatchLen = -1;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        int sepIdx = parts.indexOf("-");
        if (sepIdx == -1 || sepIdx + 2 >= parts.size()) continue;
        QString mntPoint = unescapeMountInfo(parts[4]);
        QString fsType = unescapeMountInfo(parts[sepIdx + 1]);
        QString mntSource = unescapeMountInfo(parts[sepIdx + 2]);
        if (targetPath.startsWith(mntPoint)) {
            bool isBoundary = (mntPoint.endsWith('/') || 
                               targetPath.length() == mntPoint.length() || 
                               targetPath[mntPoint.length()] == '/');
            if (isBoundary && mntPoint.length() > maxMatchLen) {
                maxMatchLen = mntPoint.length();
                outInfo.mountPoint = mntPoint;
                outInfo.fsType = fsType;
                outInfo.mountSource = mntSource;
            }
        }
    }
    return maxMatchLen > 0;
}

ResolveResult NetworkResolver::resolve(const ResolvedNode& node, ResolveContext& context) {
    QString fs = context.facts.mountInfo.fsType.toLower();
    if (fs == "nfs" || fs == "nfs4" || fs == "cifs" || fs == "smb3" || fs == "ceph") {
        context.facts.isNetwork = true;
        QString src = context.facts.mountInfo.mountSource;
        context.facts.uncPath = src;
        if (fs == "cifs" || fs == "smb3") {
            context.facts.networkProtocol = "SMB/CIFS";
        } else if (fs.startsWith("nfs")) {
            context.facts.networkProtocol = "NFS";
        } else if (fs == "ceph") {
            context.facts.networkProtocol = "CephFS";
        }
        int colonIdx = src.indexOf(':');
        if (colonIdx != -1) {
            context.facts.server = src.left(colonIdx);
            context.facts.share = src.mid(colonIdx + 1);
        } else if (src.startsWith("//") || src.startsWith("\\\\")) {
            QString cleaned = src.mid(2);
            int slashIdx = cleaned.indexOf('/');
            if (slashIdx != -1) {
                context.facts.server = cleaned.left(slashIdx);
                context.facts.share = cleaned.mid(slashIdx + 1);
            }
        }
        return ResolveResult::Finished;
    }
    return ResolveResult::NotHandled;
}