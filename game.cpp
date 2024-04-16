#include "game.h"
#include "ui_game.h"

Game::Game(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Game)
{
    ui->setupUi(this);

    generateMap();
    renderMap();

    connect (ui->checkboxEmoji, &QCheckBox::toggled, this, &Game::renderMap);
    connect (ui->buttonStartAi, &QPushButton::clicked, this, &Game::startAi);
    connect (ui->buttonStopAi, &QPushButton::clicked, this, &Game::stopAi);
    connect (ui->buttonRestart, &QPushButton::clicked, this, &Game::restart);
}

Game::~Game()
{
    delete ui;
}

void Game::generateMap()
{
    m_mapData.clear();
    int targetX = QRandomGenerator::global()->bounded(m_mapSizeX);
    int targetY = QRandomGenerator::global()->bounded(m_mapSizeY);
    for (int row = 0; row < m_mapSizeY; ++row) {
        QVector<char> rowElements;
        for (int item = 0; item < m_mapSizeX; ++item) {
            int type = QRandomGenerator::global()->bounded(10);
            char element;
            switch (type) {
            case 0: element = '#'; break;
            case 1: element = '*'; break;
            case 2: element = '!'; break;
            default: element = '.';
            }
            rowElements.push_back(element);
        }
        m_mapData.push_back(rowElements);
    }
    m_mapData[targetY][targetX] = '=';
    m_player.posx = 0;
    m_player.posy = 0;
}

void Game::renderMap()
{
    ui->map->clear();
    QString map = mapToText();
    if (ui->checkboxEmoji->isChecked() == true) {
        map.replace('@', "🐟");
        map.replace('#', "🪝");
        map.replace('*', "🐙");
        map.replace('!', "🦈");
        map.replace('.', "💧");
        map.replace('=', "🪱");
    }

    ui->map->setText(map);
}

QString Game::mapToText()
{
    QString map = "";
    for (int row = 0; row < m_mapSizeY; ++row) {
        QString currentRow = QString::fromUtf8(m_mapData[row]);
        if (row == m_player.posy)
            currentRow[m_player.posx] = '@';
        map += currentRow  + "\n";
    }
    return map;
}

void Game::restart()
{
    if (m_gameWon)
        ui->buttonStartAi->setEnabled(true);
    m_gameWon = false;
    ui->log->clear();
    ui->log->append("New game started");
    generateMap();
    renderMap();
}


bool Game::checkPlayerCanMove(int x, int y)
{
    if (x < 0 || x >= m_mapSizeX || y < 0 || y >= m_mapSizeY)
        return false;
    return m_mapData[y][x] == '.' or m_mapData[y][x] == '=';
}

void Game::processPlayerMove(QString direction)
{
    if (!m_aiActive) return;
    int x = m_player.posx;
    int y = m_player.posy;
    if (direction == "up") y -= 1;
    else if (direction == "down") y += 1;
    else if (direction == "left") x -= 1;
    else if (direction == "right") x += 1;
    else { ui->log->append("Unexpected message received: " + direction); return; }
    QString move = "ChatGPT requested to move " + direction;
    bool canMove = checkPlayerCanMove(x, y);
    if (canMove) {
        move += " and the move is made";
        if (m_mapData[y][x] == '=') {
            move += " and the game is won!";
            m_gameWon = true;
            stopAi();
            ui->buttonStartAi->setEnabled(false);
        }
        m_player.posx = x;
        m_player.posy = y;
        renderMap();
    } else
        move += " but the move is illegal";
    ui->log->append(move);
}

void Game::startAi()
{
    m_aiActive = true;
    ui->log->append("AI started");
    ui->buttonStartAi->setEnabled(false);
    ui->buttonStopAi->setEnabled(true);
    aiLoop();
}

void Game::stopAi()
{
    m_aiActive = false;
    ui->log->append("AI stopped");
    ui->buttonStartAi->setEnabled(true);
    ui->buttonStopAi->setEnabled(false);
}

void Game::aiLoop()
{
    try {
        GPT gpt;
        while (m_aiActive)
            processPlayerMove(gpt.getResponse("There is a map, with the player being marked as \"@\" character. The player can only step on \".\" and \"=\" cells. The player can only step one cell up, down, left or right in one move. The goal is to reach \"=\" character with as less moves as possible. The map will be sent below, respond to it with one word (\"up\", \"down\", \"left\", \"right\") saying what move to make. Always use lower case when responding.\n" + mapToText()));
    } catch (AccessException const&) {
        ui->log->append("Unable to access ChatGPT!");
        stopAi();
    }
}
