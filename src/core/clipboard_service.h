#ifndef CLIPBOARD_SERVICE_H
#define CLIPBOARD_SERVICE_H

#include <QObject>
#include <QString>

class ClipboardService : public QObject
{
    Q_OBJECT
public:
    static ClipboardService &instance();

    Q_INVOKABLE void copyFile(const QString &path);

private:
    explicit ClipboardService(QObject *parent = nullptr);
};

#endif // CLIPBOARD_SERVICE_H
