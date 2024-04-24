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
    bool stop = false;
    if (!m_aiActive) return;
    int x = m_player.posx;
    int y = m_player.posy;
    QStringList moves = direction.split(",");
    for (int move = 0; move < moves.length(); ++move) {
        if (moves[move] == "up") y -= 1;
        else if (moves[move] == "down") y += 1;
        else if (moves[move] == "left") x -= 1;
        else if (moves[move] == "right") x += 1;
        else { ui->log->append("Unexpected message received: " + moves[move]); break; }
        QString moveDescription = "ChatGPT requested to move " + moves[move];
        bool canMove = checkPlayerCanMove(x, y);
        if (canMove) {
            moveDescription.append(QString(" and the move is made. New position is (%1; %2).").arg(x).arg(y));
            m_player.posx = x;
            m_player.posy = y;
            renderMap();
            if (m_mapData[y][x] == '=') {
                moveDescription.append(" and the game is won!");
                m_gameWon = true;
                stopAi();
                ui->buttonStartAi->setEnabled(false);
                stop = true;
            }
        } else {
            moveDescription.append(" but the move is illegal");
            stop = true;
        }
        ui->log->append(moveDescription);
        if (stop)
            break;
    }
}

QString Game::getAiPrompt()
{
    QString prompt = QString("There is a %1x%2 field where the player has to reach the goal. Cell numbers start from 0. The player can only move one square up, down, left or right. The player cannot step on obstacles. The data is provided below:\n").arg(m_mapSizeX).arg(m_mapSizeY);

    prompt.append(QString("The player position is (%1; %2).\n").arg(m_player.posx).arg(m_player.posy));

    prompt.append("The list of obstacles: ");

    int goalX = -1;
    int goalY = -1;

    for (int row = 0; row < m_mapSizeY; ++row)
        for (int column = 0; column < m_mapSizeX; ++column)
            if (m_mapData[row][column] != '.') {
                if (m_mapData[row][column] != '=')
                    prompt += QString("(%1; %2), ").arg(column).arg(row);
                else {
                    goalX = column;
                    goalY = row;
                }
            }

    prompt.chop(2);
    prompt.append(".\n");
    prompt.append(QString("The goal position is (%1; %2).\n").arg(goalX).arg(goalY));
    prompt.append("Your response should only contain a list of comma-separated moves that need to be done to reach the goal. The list must not contain spaces, full stops, or any other characters, only letters and commas. The moves should be lowercase words \"up\", \"down\", \"left\" or \"right\". No extra words should be contained in the response.");
    return prompt;
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

void Game::aiLoop() {
    while (true) {
        try {
            GPT gpt;
            while (m_aiActive)
                processPlayerMove(gpt.getResponse(getAiPrompt()));
            break;
        } catch (AccessException const&) {
            ui->log->append("Unable to access ChatGPT!");
        }
    }
}
