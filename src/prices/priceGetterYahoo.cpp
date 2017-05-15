#include "priceGetterYahoo.h"

#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStringList>
#include <QEventLoop>
#include <QDate>
#include "historicalPrices.h"

QString priceGetterYahoo::getCSVAddress(const QString &symbol_, const QDate &beginDate_, const QDate &endDate_, const QString &type_)
{
    return QString("http://ichart.finance.yahoo.com/table.csv?s=%1&a=%2&b=%3&c=%4&d=%5&e=%6&f=%7&g=%8&ignore=.csv").arg(
        symbol_, QString::number(beginDate_.month() - 1), QString::number(beginDate_.day()), QString::number(beginDate_.year()),
                QString::number(endDate_.month() - 1), QString::number(endDate_.day()), QString::number(endDate_.year()), type_);
}

QString priceGetterYahoo::getSplitAddress(const QString &symbol)
{
    return QString("http://finance.yahoo.com/d/quotes.csv?t=my&l=on&z=l&q=l&p=&a=&c=&s=%1").arg(symbol);
}

QList<QByteArray> priceGetterYahoo::downloadFile(const QUrl &url_, bool splitResultByLineBreak_)
{
    //http://lists.trolltech.com/qt-interest/2007-11/thread00759-0.html

    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkRequest request(url_);
    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));

    loop.exec();

    QList<QByteArray> lines;
    if (reply->error() == QNetworkReply::NoError)
    {
        if (splitResultByLineBreak_)
            lines = reply->readAll().split('\n');
        else
            lines.append(reply->readAll());
    }

    delete reply;
    return lines;
}

int priceGetterYahoo::getPrices(const QString &symbol_, historicalPrices priceHistory_, int beginDate_, int endDate_) const
{
    int earliestUpdate = endDate_ + 1;
    QList<QByteArray> lines =
        downloadFile(QUrl(
            getCSVAddress(
                symbol_,
                QDate::fromJulianDay(beginDate_),
                QDate::fromJulianDay(endDate_),
                QString(stockPrices)
            )
        ));

    if (lines.count() <= 2)
        return lines.empty() ? -1 : earliestUpdate; // return true if at least the header row came through

    lines.removeFirst();
    lines.removeLast();

    foreach(const QByteArray &s, lines)
    {
        QList<QByteArray> line = s.split(','); // csv

        int date = QDate::fromString(line.at(0), Qt::ISODate).toJulianDay();
        if (priceHistory_.contains(date, historicalPrices::type_price))
            continue;

        earliestUpdate = qMin(earliestUpdate, date);
        priceHistory_.insert(date, line.at(4).toDouble(), historicalPrices::type_price);
    }

    return earliestUpdate;
}

int priceGetterYahoo::getDividends(const QString &symbol_, historicalPrices priceHistory_, int beginDate_, int endDate_) const
{
    int earliestUpdate = endDate_ + 1;
    QList<QByteArray> lines =
        downloadFile(QUrl(
            getCSVAddress(
                symbol_,
                QDate::fromJulianDay(beginDate_),
                QDate::fromJulianDay(endDate_),
                QString(stockDividends)
            )
        ));

    if (lines.count() <= 2)
        return earliestUpdate;

    lines.removeFirst();
    lines.removeLast();

    foreach(const QByteArray &s, lines)
    {
        QList<QByteArray> line = s.split(','); // csv

        int date = QDate::fromString(line.at(0), Qt::ISODate).toJulianDay();
        if (priceHistory_.contains(date, historicalPrices::type_dividend))
            continue;

        earliestUpdate = qMin(earliestUpdate, date);
        priceHistory_.insert(date, line.at(1).toDouble(), historicalPrices::type_dividend);
    }

    return earliestUpdate;
}

int priceGetterYahoo::getSplits(const QString &symbol_, historicalPrices priceHistory_, int beginDate_, int endDate_) const
{
    int earliestUpdate = endDate_ + 1;
    static const QString htmlSplitTrue = "Splits:<nobr>";  // but signifying splits
    static const QString htmlSplitNone = "Splits:none</center>"; // same line, but signifying no splits
    QList<QByteArray> lines = downloadFile(QUrl(getSplitAddress(symbol_)), false);

    if (lines.isEmpty())
        return earliestUpdate;

    QString line(lines.at(0));
    line.remove("\n").remove(" "); // shrink string

    if (line.contains(htmlSplitNone, Qt::CaseInsensitive))
        return earliestUpdate;

    int i = line.indexOf(htmlSplitTrue, 0, Qt::CaseInsensitive);
    if (i == -1)
        return earliestUpdate;
    else
        i += htmlSplitTrue.length();

    line = line.mid(i, line.indexOf("</center>", i, Qt::CaseInsensitive) - i); // read up to </center> tag
    QStringList splits = line.split("</nobr>,<nobr>");
    //the last split is missing the ",<nobr>", so we have to strip off the </nobr>"
    splits.append(splits.takeLast().remove("</nobr>"));

    foreach(const QString &s, splits)
    {
        QStringList split = s.split('[');
        QDate d = QDate::fromString(split.first(), Qt::ISODate); // try ISO format first
        if (!d.isValid()) // probably in the following format then
            d = QDate::fromString(split.first(), "MMMd,yyyy");

        if (!d.isValid())
            continue;

        int date = d.toJulianDay();
        if (date < beginDate_ || date > endDate_ || priceHistory_.contains(date, historicalPrices::type_split))
            continue;

        earliestUpdate = qMin(earliestUpdate, date);

        // ratio looks like 2:1], so strip off the last bracket
        QStringList divisor = QString(split.at(1).left(split.at(1).length() - 1)).split(':');

        if (divisor.at(0).toDouble() == 0 || divisor.at(1).toDouble() == 0) // just in case
            continue;

        double ratio = divisor.at(0).toDouble() / divisor.at(1).toDouble();

        priceHistory_.insert(date, ratio, historicalPrices::type_split);
    }

    return earliestUpdate;
}
