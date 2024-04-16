#ifndef ACCESSEXCEPTION_H
#define ACCESSEXCEPTION_H

#include <QException>

class AccessException : public QException
{
public:
    void raise() const override { throw *this; }
    AccessException *clone() const override { return new AccessException(*this); }
};

#endif // ACCESSEXCEPTION_H
