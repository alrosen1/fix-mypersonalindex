#ifndef TRADEDATECALENDAR_H
#define TRADEDATECALENDAR_H

#include <QSet>
#include <QDate>

class tradeDateCalendar
{
public:
    enum direction { direction_descending = -1, direction_ascending = 1 };

    tradeDateCalendar(int date, const direction &direction_ = direction_ascending) { m_date = checkTradeDate(date, direction_); }
    int date() { return m_date; }
    tradeDateCalendar& operator++()
    {
        m_date = checkTradeDate(++m_date, direction_ascending);
        return *this;
    }
    tradeDateCalendar operator++(int)
    {
        tradeDateCalendar copy(m_date);
        ++(*this);
        return copy;
    }


    static int checkTradeDate(int date_, direction direction_ )
    {
        int date = date_;

        while(holidays.contains(date) || date % 7 > 4) // 5 = Saturday, 6 = Sunday
            date += direction_;

        return date;
    }

    // Iterator that supports the "foreach". It needs to be
    // named *exactly* const_iterator
    // http://sites.google.com/site/andreatagliasacchi/blog/customforeachiteratorsinqt
    class const_iterator
    {
    public:
        enum etype {
            START,
            END
        };

        const_iterator(const tradeDateCalendar* v_, etype t)
        {
            endDate = QDate::currentDate().toJulianDay() + 1;

            if (t == START)
                currentDate = v_->m_date;
            if (t == END)
                currentDate = endDate;
        }

        const int operator*() { return currentDate; }
        bool operator!=(const const_iterator& it) { return currentDate != it.currentDate; }

        const_iterator& operator++()
        {
            currentDate = checkTradeDate(++currentDate, direction_ascending);

            if (currentDate > endDate)
                currentDate = endDate;

            return *this;
        }

    private:
        int currentDate; // Current iteration position
        int endDate;
    };

    const_iterator begin() const { return const_iterator(this, const_iterator::START); };
    const_iterator end() const { return const_iterator(this, const_iterator::END); };

private:
    static const QSet<int> holidays;
    int m_date;

};

#endif // TRADEDATECALENDAR_H
