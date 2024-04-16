#ifndef GAME_H
#define GAME_H

#include <QMainWindow>
#include <QRandomGenerator>
#include "gpt.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Game;
}
QT_END_NAMESPACE

struct Player {
    int posx;
    int posy;
};

class Game : public QMainWindow
{
    Q_OBJECT

public:
    Game(QWidget *parent = nullptr);
    ~Game();

public slots:
    void renderMap();

private slots:
    QString mapToText();
    void restart();
    void startAi();
    void stopAi();

private:
    void aiLoop();
    void generateMap();
    bool checkPlayerCanMove(int, int);
    void processPlayerMove(QString);

    Ui::Game *ui;

    QVector<QVector<char>> m_mapData;
    const int m_mapSizeX = 10;
    const int m_mapSizeY = 10;
    Player m_player;

    bool m_gameWon = false;
    bool m_aiActive = false;
};
#endif // GAME_H
