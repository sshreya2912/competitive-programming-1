/*
    Name: ChessMaster
    Version: 7.0
    Date: 4:34, 17 June 2013
    Creator: 3T
    Description: Based on ChessMaster v6.0.
    Update: if there's a serie of consecutive gold, try to collect from the top most
            fix something
*/


#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <queue>
#include <set>
#include <cstdio>

using namespace std;

#define fr(i,a,b) for (int i = (a), _b = (b); i <= _b; i++)
#define frr(i,a,b) for (int i = (a), _b = (b); i >= _b; i--)
#define rep(i,n) for (int i = 0, _n = (n); i < _n; i++)
#define repr(i,n) for (int i = (n) - 1; i >= 0; i--)
#define mp make_pair
#define ff first
#define ss second
#define INF 1000111222
#define pb push_back

typedef pair<int, int> ii;
typedef pair<ii, ii> i4;
typedef pair <int, ii> iii;

// Size of the playing field.
const int FIELD_ROW_COUNT = 16;
const int FIELD_COLUMN_COUNT = 25;

// How close an enemy can be before we try to move away or trap it.
// 5/6/2013: change from 5 to 3
const int THREAT_THRESHOLD = 3;
const int THREAT_MIN = 2;
const int RESPAWN_ENEMY = 25;
const int RESPAWN_RUNNER = 50;
const int RESPAWN_BRICK = 25;
const int RESPAWN_GOLD = 150;

// Constants for the various directions the player can move.
enum movedir {N, L, T, R, B};
enum MoveDir { NONE, LEFT, TOP, RIGHT, BOTTOM, DIG_LEFT, DIG_RIGHT };

// Names for the different directions.
string moveName[] = { "NONE", "LEFT", "TOP", "RIGHT", "BOTTOM", "DIG_LEFT", "DIG_RIGHT" };

// Row and column offsets for the move directions.
int dRow[] = {0, 0, -1, 0, 1 };
int dCol[] = {0, -1, 0, 1, 0 };

/** Representation for a player or enemy position. */
struct Position {
  int r, c;

  Position() : r( 0 ), c( 0 ) {
  }
};

Position P0;

string world[ FIELD_ROW_COUNT ];
string prevworld[FIELD_ROW_COUNT];
int availGold[FIELD_ROW_COUNT + 5][FIELD_COLUMN_COUNT + 5];
int khung[FIELD_ROW_COUNT + 5][FIELD_COLUMN_COUNT + 5];
int goldActive[FIELD_ROW_COUNT][FIELD_COLUMN_COUNT];

int d[FIELD_ROW_COUNT][FIELD_COLUMN_COUNT];
set <iii> DSet;
vector <iii> EnemyCircuit[555];
  int enemyCount;
  // Respawn spot for each enemy.
  vector< Position > initEnemies( 555 );
  vector<int> enemyActive(555);

  // What player each enemy is afraid of (or -1), not used in this player.
  vector< int > master( 555, -1 );

  // Circuit the enemy will make while patrolling.  Not useed in this player.
  vector< string > program( 555 );

  // Initial positions and current positions for each runner.
  Position initRunners[ 2 ], runners[ 2 ], prevRunners[2];

  int oppoMoveDir;

  // Current score in the game.
  int scores[ 2 ];

  // How long until each runner can dig.
  int digDelay[ 2 ];

  // How long will the game be?
  int turns;

    // Read the turn number (should be [ 0, turns ) unless we're late with a move.
    int turn;

  // Time when we last visited this space, or -1.
  vector< vector< int > > visitTime( FIELD_ROW_COUNT, vector< int >( FIELD_COLUMN_COUNT, -1 ) );

   // Current enemy positions.
  vector< Position > enemies;

  // Last known move direction for each enemy.
  vector< int > enemyMoveDir( 555 );

/** Simple interface for reading positions from standard input. */
istream &operator>>( istream &input, Position &pos ) {
  input >> pos.r >> pos.c;
  return input;
}

/** Return true if a player will fall through this map character. */
bool isAir( char ch ) {
  return !( ch == '=' || ch == 'H' );
}

// 5/6/2013: If no enemy is at a cell at the moment
bool nobodyAtPosition(int x, int y, vector< Position > v) {
    rep(i, v.size()) {
        if(v[i].r == x && v[i].c == y) return false;
    }
    return true;
}

// 6/6/2013: check empty cell
bool isEmptyLike(int x, int y, vector< Position > v) {
    if(world[x][y] == '=' || world[x][y] == 'H') return false;
    if(world[x][y] == '-' && !nobodyAtPosition(x, y, v)) return false;
    return true;
}

// 5/6/2013: check whether an enemy is a threat or not
bool isThreat(Position runn, Position enn, vector< Position > v) {
    if(world[enn.r][enn.c] == '-') return false;
    if(runn.r == enn.r) {
        for(int i = min(runn.c, enn.c); i <= max(runn.c, enn.c); i++) if(world[runn.r][i] == '=' || (runn.r + 1 < FIELD_ROW_COUNT && world[runn.r + 1][i] == '-' && nobodyAtPosition(runn.r + 1, i, v))) return false;
    }
    if(runn.c == enn.c) {
        for(int i = min(runn.r, enn.r); i <= max(runn.r, enn.r); i++) if(world[i][runn.c] != 'H') return false;
    }
    return true;
}

// 6/6/2013: check whether runner can dig right if he's at pos x, y
bool canDigRightAtPos(Position pos) {
    if(pos.c + 1 >= FIELD_COLUMN_COUNT) return false;
    if ( pos.r + 1 < FIELD_ROW_COUNT ) {
          if ( world[ pos.r ][ pos.c + 1 ] != 'H' && world[ pos.r ][ pos.c + 1 ] != '=' && world[ pos.r + 1 ][ pos.c + 1 ] == '=' &&
               ( world[ pos.r + 1 ][ pos.c ] == '=' || world[ pos.r + 1 ][ pos.c ] == 'H' || world[pos.r][pos.c] == 'H') )
            return true;
        }
    return false;
}

// 6/6/2013: check whether runner can dig left if he's at pos x, y
bool canDigLeftAtPos(Position pos) {
    if(pos.c - 1 < 0) return false;
    if ( pos.r + 1 < FIELD_ROW_COUNT ) {
          if ( world[ pos.r ][ pos.c - 1 ] != 'H' && world[ pos.r ][ pos.c - 1 ] != '=' && world[ pos.r + 1 ][ pos.c - 1 ] == '=' &&
               ( world[ pos.r + 1 ][ pos.c ] == '=' || world[ pos.r + 1 ][ pos.c ] == 'H' || world[pos.r][pos.c] == 'H') )
            return true;
        }
    return false;
}

// 6/6/2013: count how many nearby enemy (not working)
/*
int countNearbyEnemy(int x, int y, vector< Position > v){
    int res = 0;
    return 0;
    rep(i, v.size()) {
        if(abs(v[i].r - x) + abs(v[i].c - y) <= 2) res++;
    }
    return res;
}
*/

// 6/6/2013: check whether a position is inside the map
bool insideMap(int x, int y) {
    return (x >= 0 && x < FIELD_ROW_COUNT && y >= 0 && y < FIELD_COLUMN_COUNT);
}

//17/6/2013: don't work
bool fallintoPatrolled(int x, int y) {
    return false;
    if(world[x][y] == 'H') return false;
    int xx = FIELD_ROW_COUNT - 1;
    fr(i, x + 1, FIELD_ROW_COUNT - 1) {
        if(!isEmptyLike(i, y, enemies)) {
            xx = i;
            break;
        }
    }
    xx--;

    x = xx;
    int cnt = 0;
    rep(i, 7) {
        int ok = 0;
        if(ok) break;
        if(y + i == FIELD_COLUMN_COUNT) break;
        if(!isEmptyLike(x, y + i, enemies) || (x + 1 < FIELD_ROW_COUNT && isEmptyLike(x + 1, y + i, enemies))) break;
        rep(k, enemyCount) if(enemies[k].r == x && enemies[k].c == y + i) {
            ok = 1;
            cnt++;
            break;
        }
    }

    rep(i, 7) {
    int ok = 0;
    if(ok) break;
    if(y - i < 0) break;
    if(!isEmptyLike(x, y - i, enemies) || (x + 1 < FIELD_ROW_COUNT && isEmptyLike(x + 1, y - i, enemies))) break;
    rep(k, enemyCount) if(enemies[k].r == x && enemies[k].c == y - i) {
        ok = 1;
        cnt++;
        break;
        }
    }
    return cnt == 2;
}

// 6/6/2013: get distance to the nearest gold from pos
ii getMinDistanceToGold(int x, int y, vector <Position> v, int MoveType) {
    char tmpch = world[x][y];
    if(MoveType == DIG_LEFT || MoveType == DIG_RIGHT) world[x][y] = '.';

    //initiation
    int res = INF;
    /*
    fr(i, x, FIELD_ROW_COUNT - 1) rep(j, FIELD_COLUMN_COUNT) {
        if(world[i][j] == '*')
        res = min(res, i - x + abs(j - y));
    }*/

    vector< pair<ii, int> > adj[FIELD_ROW_COUNT][FIELD_COLUMN_COUNT];
    rep(i, FIELD_ROW_COUNT) rep(j, FIELD_COLUMN_COUNT) {
        //falling
        if(world[i][j] != 'H' && i + 1 < FIELD_ROW_COUNT && isEmptyLike(i + 1, j, v)) {
            adj[i][j].pb(mp(mp(i + 1, j), 1));
            continue;
        }

        // 9/6/2013:
        //whether we can go to 4 directions from current cell or stay
        rep(k, 5) {
            if(!k) {adj[i][j].pb(mp(mp(i, j), 1)); continue;}
            int xx = i + dRow[k], yy = j + dCol[k];
            if(!insideMap(xx, yy)) continue;
            if(k == LEFT || k == RIGHT) {
                if(world[xx][yy] != '=' && !fallintoPatrolled(xx, yy)) adj[i][j].pb(mp(mp(xx, yy), 1));
            }
            else if(k == BOTTOM || k == TOP) {
                if(world[i][j] == 'H' && world[xx][yy] != '=' && !fallintoPatrolled(xx, yy)) adj[i][j].pb(mp(mp(xx, yy), 1));
            }
        }

        //whether we can dig right or dig left from current cell
        Position tmpPos; tmpPos.r = i; tmpPos.c = j;
        if(canDigLeftAtPos(tmpPos) && !fallintoPatrolled(i + 1, j -1)) adj[i][j].pb(mp(mp(i + 1, j - 1), 3));
        if(canDigRightAtPos(tmpPos) && !fallintoPatrolled(i + 1, j + 1)) adj[i][j].pb(mp(mp(i + 1, j + 1), 3));
    }

    DSet.clear();
    rep(i, FIELD_ROW_COUNT) rep(j, FIELD_COLUMN_COUNT) d[i][j] = INF;
    d[x][y] = 0;
    DSet.insert(mp(0, mp(x, y)));

    //run dijkstra
    while(!DSet.empty()) {
        iii tmp = *DSet.begin();
        DSet.erase(tmp);

        int ux = tmp.ss.ff, uy = tmp.ss.ss;
        rep(i, adj[ux][uy].size()) {
            int vx = adj[ux][uy][i].ff.ff, vy = adj[ux][uy][i].ff.ss, cost = adj[ux][uy][i].ss;
            if(d[vx][vy] > d[ux][uy] + cost) {
                if(d[vx][vy] != INF) DSet.erase(mp(d[vx][vy], mp(vx, vy)));
                d[vx][vy] = d[ux][uy] + cost;
                DSet.insert(mp(d[vx][vy], mp(vx, vy)));
            }
        }
    }
/*
    rep(i, FIELD_ROW_COUNT) rep(j, FIELD_COLUMN_COUNT) {
        if(world[i][j] == '*') res = min(res, d[i][j]);
    }
*/

    //9/6/2013: can run to the next respawn gold
    // 11/6/2013: run to the next repawn gold and wait there
    int res2 = INF, res1 = INF, idx = -1, idy = -1;
    rep(i, FIELD_ROW_COUNT) rep(j, FIELD_COLUMN_COUNT) {
        if(goldActive[i][j] == -1 || !availGold[i][j]) continue;
        //res1 = min(res1, max(d[i][j], goldActive[i][j] + 1));
        int tmp = max(d[i][j], goldActive[i][j] + 1);
        if(tmp < res1) {
            res1 = tmp;
            res2 = d[i][j];
        }
        else if(tmp == res1 && d[i][j] < res2) {
            res1 = tmp;
            res = d[i][j];
        }
        //res2 += max(d[i][j], goldActive[i][j] + 1);
    }


    world[x][y] = tmpch;

//    res2 = 0;

    return ii(res1, res2);
}

// 6/6/2013: check whether enemies can reach the cell in a move
bool isAround(int x, int y) {
    //return false;
    rep(i, enemies.size()) {
        if(enemies[i].r == -1 && enemies[i].c == -1) {
            if(initEnemies[i].r == x && initEnemies[i].c == y && enemyActive[i] <= 1) return true; //respawn in a move
            continue;
        }
        if(enemies[i].r == x && enemies[i].c == y) return true;
        if(world[enemies[i].r][enemies[i].c] == '-') continue;
        if(enemies[i].r + 1 < FIELD_ROW_COUNT && isEmptyLike(enemies[i].r + 1, enemies[i].c, enemies) && world[enemies[i].r][enemies[i].c] != 'H') continue;
        if(enemies[i].r == x - 1 && enemies[i].c == y) return true;
        if(enemies[i].r == x + 1 && enemies[i].c == y && world[enemies[i].r][enemies[i].c] == 'H') return true;
        if(enemies[i].r == x && enemies[i].c == y + 1) return true;
        if(enemies[i].r == x && enemies[i].c == y - 1) return true;
    }
    return false;
}

int convert(char ch) {
    if(ch == 'L') return LEFT;
    if(ch =='T') return TOP;
    if(ch == 'B') return BOTTOM;
    if(ch == 'R') return RIGHT;
}

void gen() {
    P0.r = -1; P0.c = -1;
    /*
    rep(i, enemyCount) {
        int curx = enemies[i].r, cury = enemies[i].c;
        rep(j, program[i].length()) {
            int tmp = convert(program[i][j]);
            EnemyCircuit[i].pb(mp(tmp, mp(curx, cury)));
            curx += dRow[tmp]; cury += dCol[tmp];
        }
    }*/

    rep(i, enemyCount) enemyActive[i] = 0;

    rep(i, FIELD_ROW_COUNT) rep(j, FIELD_COLUMN_COUNT) {
        if(world[i][j] == '*') goldActive[i][j] = 0;
        else goldActive[i][j] = -1;
    }
}

// 9/6/2013: check whether a position will fall into an enemy
bool fallIntoEnemy(int x, int y) {
    //return false;
    if(world[x][y] == 'H') return false;
    int xx = FIELD_ROW_COUNT - 1;
    fr(i, x + 1, FIELD_ROW_COUNT - 1) {
        if(!isEmptyLike(i, y, enemies)) {
            xx = i;
            break;
        }
    }
    xx--;

    int step = xx - x + 1;
    step /= 2;
    int st = step + 2;

//    rep(i, enemyCount) {
//        bool conf = false;
//        rep(j, program[i].length()) if(EnemyCircuit[i][j].ss.ff == xx && EnemyCircuit[i][j].ss.ss == y) {
//            conf = true;
//            break;
//        }
//        if(!conf) continue;
//        int idx = -1;
//        rep(j, program[i].length()) if(EnemyCircuit[i][j].ss.ff == enemies[i].r && EnemyCircuit[i][j].ss.ss == enemies[i].c && EnemyCircuit[i][j].ff == enemyMoveDir[i]) {
//            idx = j;
//        }
//        if(idx == -1) continue;
//
//        while(st--) {
//            idx++; if(idx == program[i].length()) idx = 0;
//        }
//        if(abs(EnemyCircuit[i][idx].ss.ff - xx) <= 1 && EnemyCircuit[i][idx].ss.ss == y) return true;
        //return false;
        //
        rep(j, st) {
            if(y + j >= FIELD_COLUMN_COUNT) break;
            if(xx + 1 < FIELD_ROW_COUNT && isEmptyLike(xx + 1, y + j, enemies)) break;
            rep(k, enemyCount) if(enemies[k].r == xx && enemies[k].c == y + j) return true;
        }
        rep(j, st) {
            if(y - j < 0) break;
            if(xx + 1 < FIELD_ROW_COUNT && isEmptyLike(xx + 1, y - j, enemies)) break;
            rep(k, enemyCount) if(enemies[k].r == xx && enemies[k].c == y - j) return true;
        }
//    }

    return false;
}

// 9/6/2013: can escape from a cell, temporarily just return whether it is on the second last rank or not
bool canEscape(int x, int y) {
    //return true;
    if(turns - turn <= 50) return true;

    if(x < FIELD_ROW_COUNT - 2) return true;

    //17/6/2013: if this is the last row
    if(x == FIELD_ROW_COUNT - 1) {
        rep(i, FIELD_COLUMN_COUNT) {
            if(y + i == FIELD_COLUMN_COUNT || world[x][y + i] == '=') break;
            rep(j, enemyCount) if(enemies[j].r == x && enemies[j].c == y + i) return false;
            if(world[x][y + i] == 'H') return true;
        }
        rep(i, FIELD_COLUMN_COUNT) {
            if(y - i < 0 || world[x][y - i] == '=') break;
            rep(j, enemyCount) if(enemies[j].r == x && enemies[j].c == y - i) return false;
            if(world[x][y - i] == 'H') return true;
        }
        return false;
    }
    rep(i, FIELD_COLUMN_COUNT) {
        if(y + i == FIELD_COLUMN_COUNT || world[x][y + i] == '=') break;
        if(world[x][y + i] == 'H') return true;
    }

    rep(i, FIELD_COLUMN_COUNT) {
        if(y - i < 0 || world[x][y - i] == '=') break;
        if(world[x][y - i] == 'H') return true;
    }
    return false;
}

//11/6/2013: fall into somewhere but no escape
bool fallIntoNoEscape(int x, int y) {
    //return false;

    if(world[x][y] == 'H') return false;
    int xx = FIELD_ROW_COUNT - 1;
    fr(i, x + 1, FIELD_ROW_COUNT - 1) {
        if(!isEmptyLike(i, y, enemies)) {
            xx = i;
            break;
        }
    }
    xx--;
    if(xx < FIELD_ROW_COUNT - 2) return false;
    //can not escape
    if(!canEscape(xx, y)) return true;
    return false;
}

// 9/6/2013: patrolled by two enemies at two ends
bool isPatrolled(int x, int y) {
    //return false;
    int cnt = 0;
    rep(i, 5) {
        int ok = 0;
        if(ok) break;
        if(y + i == FIELD_COLUMN_COUNT) break;
        if(!isEmptyLike(x, y + i, enemies) || (x + 1 < FIELD_ROW_COUNT && isEmptyLike(x + 1, y + i, enemies))) break;
        rep(k, enemyCount) if(enemies[k].r == x && enemies[k].c == y + i) {
            ok = 1;
            cnt++;
            break;
        }
    }

    rep(i, 5) {
    int ok = 0;
    if(ok) break;
    if(y - i < 0) break;
    if(!isEmptyLike(x, y - i, enemies) || (x + 1 < FIELD_ROW_COUNT && isEmptyLike(x + 1, y - i, enemies))) break;
    rep(k, enemyCount) if(enemies[k].r == x && enemies[k].c == y - i) {
        ok = 1;
        cnt++;
        break;
        }
    }
    return cnt == 2;
}

//17/6/2013: is safe or can not escape from enemies
bool isSafe(int x, int y) {
    //return true;
        //falling
        if(world[x][y] != 'H' && x + 1 < FIELD_ROW_COUNT && isEmptyLike(x + 1, y, enemies)) {
            return true;
        }

        rep(k, 5) {
            if(!k) {if(!isAround(x, y)) return true;}
            int xx = x + dRow[k], yy = y + dCol[k];
            if(!insideMap(xx, yy)) continue;
            if(k == LEFT || k == RIGHT) {
                if(world[xx][yy] != '=' && !isAround(xx, yy)) return true;
            }
            else if(k == BOTTOM || k == TOP) {
                if(world[x][y] == 'H' && world[xx][yy] != '=' && !isAround(xx, yy)) return true;
            }
        }
}

//17/6/2013
Position getOpponentGoldTarget(int x, int y, vector< Position > v) {
    // 13/7/2013: stupid here :((
    if(x == -1 && y == -1) return P0;
    //initiation
    int res = INF;
    /*
    fr(i, x, FIELD_ROW_COUNT - 1) rep(j, FIELD_COLUMN_COUNT) {
        if(world[i][j] == '*')
        res = min(res, i - x + abs(j - y));
    }*/

    vector< pair<ii, int> > adj[FIELD_ROW_COUNT][FIELD_COLUMN_COUNT];
    rep(i, FIELD_ROW_COUNT) rep(j, FIELD_COLUMN_COUNT) {
        //falling
        if(world[i][j] != 'H' && i + 1 < FIELD_ROW_COUNT && isEmptyLike(i + 1, j, v)) {
            adj[i][j].pb(mp(mp(i + 1, j), 1));
            continue;
        }

        // 9/6/2013:
        //whether we can go to 4 directions from current cell or stay
        rep(k, 5) {
            if(!k) {adj[i][j].pb(mp(mp(i, j), 1)); continue;}
            int xx = i + dRow[k], yy = j + dCol[k];
            if(!insideMap(xx, yy)) continue;
            if(k == LEFT || k == RIGHT) {
                if(world[xx][yy] != '=') adj[i][j].pb(mp(mp(xx, yy), 1));
            }
            else if(k == BOTTOM || k == TOP) {
                if(world[i][j] == 'H' && world[xx][yy] != '=') adj[i][j].pb(mp(mp(xx, yy), 1));
            }
        }

        //whether we can dig right or dig left from current cell
        Position tmpPos; tmpPos.r = i; tmpPos.c = j;
        if(canDigLeftAtPos(tmpPos)) adj[i][j].pb(mp(mp(i + 1, j - 1), 3));
        if(canDigRightAtPos(tmpPos)) adj[i][j].pb(mp(mp(i + 1, j + 1), 3));
    }

    DSet.clear();
    rep(i, FIELD_ROW_COUNT) rep(j, FIELD_COLUMN_COUNT) d[i][j] = INF;
    d[x][y] = 0;
    DSet.insert(mp(0, mp(x, y)));

    //run dijkstra
    while(!DSet.empty()) {
        iii tmp = *DSet.begin();
        DSet.erase(tmp);

        int ux = tmp.ss.ff, uy = tmp.ss.ss;
        rep(i, adj[ux][uy].size()) {
            int vx = adj[ux][uy][i].ff.ff, vy = adj[ux][uy][i].ff.ss, cost = adj[ux][uy][i].ss;
            if(d[vx][vy] > d[ux][uy] + cost) {
                if(d[vx][vy] != INF) DSet.erase(mp(d[vx][vy], mp(vx, vy)));
                d[vx][vy] = d[ux][uy] + cost;
                DSet.insert(mp(d[vx][vy], mp(vx, vy)));
            }
        }
    }
/*
    rep(i, FIELD_ROW_COUNT) rep(j, FIELD_COLUMN_COUNT) {
        if(world[i][j] == '*') res = min(res, d[i][j]);
    }
*/

    //9/6/2013: can run to the next respawn gold
    // 11/6/2013: run to the next repawn gold and wait there
    int res2 = INF, res1 = INF, idx = -1, idy = -1;
    Position ansp; //ansp.r = -1; ansp.c = -1;
    rep(i, FIELD_ROW_COUNT) rep(j, FIELD_COLUMN_COUNT) {
        if(goldActive[i][j] == -1) continue;
        //res1 = min(res1, max(d[i][j], goldActive[i][j] + 1));
        int tmp = max(d[i][j], goldActive[i][j] + 1);
        if(tmp < res1) {
            res1 = tmp;
            res2 = d[i][j];
            ansp.r = i; ansp.c = j;
        }
        else if(tmp == res1 && d[i][j] < res2) {
            res1 = tmp;
            res = d[i][j];
            ansp.r = i; ansp.c = j;
        }
        //res2 += max(d[i][j], goldActive[i][j] + 1);
    }


//    res2 = 0;

    return ansp;
}

// 9/6/2013: pre process gold
// 13/7/2013
void preProcessGold() {

    //17/6/2013: find opponent's gold target
    int etx, ety;
    Position shit2 = getOpponentGoldTarget(runners[1].r, runners[1].c, enemies);
    etx = shit2.r; ety = shit2.c;
    //etx = -1; ety = -1;

    //rep(i, FIELD_ROW_COUNT) rep(j, FIELD_COLUMN_COUNT) availGold[i][j] = -1;

    rep(i, FIELD_ROW_COUNT) rep(j, FIELD_COLUMN_COUNT) {
        if(i == etx && j == ety) {
            availGold[i][j] = 0;
            continue;
        }
        if(goldActive[i][j] == -1) {availGold[i][j] = 0; continue;}
        if(!goldActive[i][j] && world[i][j] != '*') goldActive[i][j] = RESPAWN_GOLD;
        if(goldActive[i][j] && world[i][j] != '*') goldActive[i][j] = max(goldActive[i][j] - 1, 0);
        if(goldActive[i][j] && world[i][j] == '*') goldActive[i][j] = 0;

        //if(availGold[i][j] != -1) continue;
        availGold[i][j] = ((canEscape(i, j) && !isPatrolled(i, j))) ? 1 : 0;
    }

    //set gold below where we can fall from a gold into unavailable
    rep(i, FIELD_ROW_COUNT) rep(j, FIELD_COLUMN_COUNT) {
        if(goldActive[i][j] == -1) continue;
        if(!availGold[i][j]) continue;
        int tmp = i + 1, damn = 0;
        while(tmp < FIELD_ROW_COUNT && isEmptyLike(tmp, j, enemies)) {
            if(goldActive[tmp][j] != -1) availGold[tmp][j] = 0;
            tmp++;
        }
    }
}

// 11/6/2013: nobody aside:
bool nobodyAside(int x, int y, int d) {
    //return true;
    if(d == LEFT) {
        rep(i, enemyCount) {
            if(enemies[i].r == x && enemies[i].c == y - 1) return false;
        }
        return true;
    }
    if(d == RIGHT) {
        rep(i, enemyCount) {
            if(enemies[i].r == x && enemies[i].c == y + 1) return false;
        }
        return true;
    }
}

int main( int argc, char* argv[] ) {
  cin >> turns;

  // Read a picture of the map.
  // 6/6/2013: world is now a global array (line 44)
  for (int i = 0; i < FIELD_ROW_COUNT; i++)
    cin >> world[ i ];

  // Read initial positions of the two runners.
  for(int i = 0; i < 2; i++)
    cin >> initRunners[i];

  // Read initial positions of all the enemies.
  cin >> enemyCount;

  for(int i = 0; i < enemyCount; i++)
    cin >> initEnemies[i] >> program[ i ];

    //enemies = initEnemies;
    Position shit;
    rep(i, enemyCount) shit = initEnemies[i], enemies.pb(shit);

    // 9/6/2013
    gen();

  // For each turn in the game.
  for ( int turnNumber = 0; turnNumber < turns; turnNumber++ ) {
    cin >> turn;


    // Read the current world description, since gold may be collected
    // and the floor may be destroyed.
    for (int i = 0; i < FIELD_ROW_COUNT; i++)
      cin >> world[ i ];

    // Read the positions of each player's runners, current scores and
    // how long until each can dig.
    rep(i, 2) prevRunners[i].r = runners[i].r, prevRunners[i].c = runners[i].c;
    for(int i = 0; i < 2; i++)
      cin >> runners[i] >> scores[ i ] >> digDelay[ i ];

    // For convenience, remember my position in some local variables.
    int myRow = runners[ 0 ].r;
    int myCol = runners[ 0 ].c;

    // Remember previous enemy positions, and read latest ones.
    vector< Position > prevEnemies;
    rep(i, enemyCount) shit = enemies[i], prevEnemies.pb(shit);
    for ( int i = 0; i < enemyCount; i++ )
      cin >> enemies[ i ] >> master[ i ];

    //init
    rep(i, enemyCount) {
        if(enemies[i].r == -1 && prevEnemies[i].r == -1) enemyActive[i] = max(enemyActive[i] - 1, 0);
        else if(enemies[i].r == -1 && prevEnemies[i].r != -1) enemyActive[i] = RESPAWN_ENEMY;
    }

   //pre-process gold
    preProcessGold();

    // Tentative move.  See if we can do better.
    string move = "NONE";
    if ( myRow >= 0 && myCol >= 0 ) {

      // Remember when I visited this cell.
      visitTime[ myRow ][ myCol ] = turn;

      // Is there an enemy heading toward us from the left or right?
      bool approachingLeft = false, approachingRight = false, isLeft = false, isRight = false, isBottom = false, isTop = false;

      // Can we move left, up, right or down?
      vector< bool > canMove( 10, false );

      // Is there a nearby enemy left, up, right or down?
      vector< bool > threat( 10, false );


        // 11/6/2013: opponent move direction
        if ( runners[ 1 ].r > prevRunners[ 1 ].r )
          oppoMoveDir = BOTTOM;
        else if ( runners[ 1 ].r < prevRunners[ 1 ].r )
          oppoMoveDir = TOP;
        else if ( runners[ 1 ].c > prevRunners[ 1 ].c )
          oppoMoveDir = RIGHT;
        else if ( runners[ 1 ].c < prevRunners[ 1 ].c )
          oppoMoveDir = LEFT;
        else oppoMoveDir = NONE;

      for ( int i = 0; i < enemyCount; i++ ) {
        // If this enemy has moved, remember its move direction.
        if ( enemies[ i ].r > prevEnemies[ i ].r )
          enemyMoveDir[ i ] = BOTTOM;
        else if ( enemies[ i ].r < prevEnemies[ i ].r )
          enemyMoveDir[ i ] = TOP;
        else if ( enemies[ i ].c > prevEnemies[ i ].c )
          enemyMoveDir[ i ] = RIGHT;
        else if ( enemies[ i ].c < prevEnemies[ i ].c )
          enemyMoveDir[ i ] = LEFT;

        if(enemies[i].r == myRow - 1 && enemies[i].c == myCol) isTop = true;
        if(enemies[i].r == myRow + 1 && enemies[i].c == myCol) isBottom = true;
        if(enemies[i].r == myRow && enemies[i].c == myCol - 1) isLeft = true;
        if(enemies[i].r == myRow && enemies[i].c == myCol + 1) isRight = true;

        // See if there are enemies nearby
        //5/6/2013: update threat (if there's already hole, don't worry)
        if ( enemies[ i ].r == myRow && enemies[ i ].c < myCol &&
             ((myCol - enemies[ i ].c < THREAT_THRESHOLD && isThreat(runners[0], enemies[i], enemies)) || myCol - enemies[i].c <= THREAT_MIN)) {
          threat[ LEFT ] = true;

          if ( myCol - enemies[ i ].c >= 1 && enemyMoveDir[ i ] == RIGHT )
            approachingLeft = true;
        }

        if ( enemies[ i ].r == myRow && enemies[ i ].c > myCol &&
             ((enemies[ i ].c - myCol < THREAT_THRESHOLD && isThreat(runners[0], enemies[i], enemies)) || enemies[i].c - myCol <= THREAT_MIN)) {
          threat[ RIGHT ] = true;

          if ( enemies[ i ].c - myCol >= 1 && enemyMoveDir[ i ] == LEFT )
            approachingRight = true;
        }

        if ( enemies[ i ].c == myCol && enemies[ i ].r < myRow &&
             myRow - enemies[ i ].r < THREAT_THRESHOLD && isThreat(runners[0], enemies[i], enemies))
          threat[ TOP ] = true;

        if ( enemies[ i ].c == myCol && enemies[ i ].r > myRow &&
             enemies[ i ].r - myRow < THREAT_THRESHOLD && isThreat(runners[0], enemies[i], enemies))
          threat[ BOTTOM ] = true;
      }

      // Can we dig left or right?
      bool canDig = ( digDelay[ 0 ] == 0 );
      bool canDigLeft = false, canDigRight = false;

      // Look around the runner and see what it can do.
      if ( myCol > 0 && ((world[ myRow ][ myCol - 1 ] != '=' && world[ myRow ][ myCol - 1 ] != '-' )/*|| (world[myRow][myCol - 1] == '-' && nobodyAtPosition(myRow, myCol - 1, enemies))*/) /*&& isEmptyLike(myRow, myCol - 1, enemies)*/) {
        canMove[ LEFT ] = true;

        //5/6/2013: don't move if you fall into a removed brick at bottom
        if(myRow + 1 == FIELD_ROW_COUNT - 1 && world[myRow + 1][myCol - 1] == '-' && nobodyAtPosition(myRow + 1, myCol - 1, enemies))
        canMove[LEFT] = false;

        // 9/6/2013: don't move if you fall into an moving enemy
        if(myRow + 1 < FIELD_ROW_COUNT && isEmptyLike(myRow + 1, myCol - 1, enemies) && fallIntoEnemy(myRow, myCol - 1)) canMove[LEFT] = false;

        if(canDigLeftAtPos(runners[0])) canDigLeft = canDig;
        }

      if ( myCol + 1 < FIELD_COLUMN_COUNT && ((world[ myRow ][ myCol + 1 ] != '=' && world[ myRow ][ myCol + 1 ] != '-')/*|| (world[myRow][myCol + 1] == '-' && nobodyAtPosition(myRow, myCol + 1, enemies))*/) /*&& isEmptyLike(myRow, myCol + 1, enemies)*/) {
        canMove[ RIGHT ] = true;

        //5/6/2013: don't move if you fall into a removed brick at bottom
        if(myRow + 1 == FIELD_ROW_COUNT - 1 && world[myRow + 1][myCol + 1] == '-' && nobodyAtPosition(myRow + 1, myCol + 1 , enemies))
        canMove[RIGHT] = false;

        // 9/6/2013: don't move if you fall into an moving enemy
        if(myRow + 1 < FIELD_ROW_COUNT && isEmptyLike(myRow + 1, myCol + 1, enemies) && fallIntoEnemy(myRow, myCol + 1)) canMove[RIGHT] = false;

        if(canDigRightAtPos(runners[0])) canDigRight = canDig;
      }

      if ( myRow > 0 && world[ myRow ][ myCol ] == 'H' && ((world[ myRow - 1 ][ myCol ] != '=' && world[ myRow - 1 ][ myCol ] != '-' )|| (world[myRow -1][myCol] == '-' && nobodyAtPosition(myRow - 1, myCol, enemies))) /*&& isEmptyLike(myRow - 1, myCol, enemies)*/) {
        canMove[ TOP ] = true;
      }
      // 5/6/2013: if there's something in the hole you cannot go down
      if ( myRow + 1 < FIELD_ROW_COUNT && ((world[ myRow + 1 ][ myCol ] != '=' && world[ myRow + 1 ][ myCol ] != '-' )|| (world[myRow + 1][myCol] == '-' && nobodyAtPosition(myRow + 1, myCol, enemies))) /*&& isEmptyLike(myRow + 1, myCol, enemies)*/) {
        canMove[ BOTTOM ] = true;
        if(world[ myRow + 1 ][ myCol ] == '-' && (!nobodyAtPosition(myRow + 1, myCol, enemies) || (runners[1].r == myRow + 1 && runners[1].c == myCol))) canMove[BOTTOM] = false;
        if(isEmptyLike(myRow + 1, myCol, enemies) && fallIntoEnemy(myRow + 1, myCol)) canMove[BOTTOM] = false;
      }

      // First, try to trap an enemy if we can.
      // 5/6/2013: dig when you are threaten
      //need update dig when you are threaten or opponent approaching towards
      //11/6/2013: if fall to somewhere we can't escape, let the enemies kill us
      if ( canDigLeft && approachingLeft && threat[LEFT] && canEscape(myRow, myCol) && nobodyAside(myRow, myCol, LEFT)) {
        move = "DIG_LEFT";
      } else if ( canDigRight && approachingRight && threat[RIGHT] && canEscape(myRow, myCol) && nobodyAside(myRow, myCol, RIGHT)) {
        move = "DIG_RIGHT";
      }
/*debug
        if(canMove[LEFT] && world[myRow + dRow[LEFT] + 1][myCol + dCol[LEFT]] != '=' && !isAround(myRow + dRow[LEFT], myCol + dCol[LEFT])) {
            move = "LEFT";
        }
        else
*/
      //if ( canDigLeft && approachingLeft && threat[LEFT] && canEscape(myRow, myCol)) {
       // move = "DIG_LEFT";
      //} else if ( canDigRight && approachingRight && threat[RIGHT] && canEscape(myRow, myCol)) {
      //  move = "DIG_RIGHT";
      //}
      // 11/6/2013: try to trap opponent
      else if(insideMap(runners[1].r + dRow[oppoMoveDir], runners[1].c + dCol[oppoMoveDir]) &&
              canDigLeft && runners[0].r == runners[1].r + dRow[oppoMoveDir] && runners[0].c == runners[1].c + dCol[oppoMoveDir] + 1)
              move = "DIG_LEFT";
      else if(insideMap(runners[1].r + dRow[oppoMoveDir], runners[1].c + dCol[oppoMoveDir]) &&
              canDigRight && runners[0].r == runners[1].r + dRow[oppoMoveDir] && runners[0].c == runners[1].c + dCol[oppoMoveDir] - 1)
              move = "DIG_RIGHT";
      else {
        vector< i4 > moves;

        // First, try to select from threat-free moves.
        // 6/6/2013: count number of nearby enemy, but it seems not work
        // try to go to the nearest gold
        for ( int i = 1; i <= 4; i++ )
          if ( canMove[ i ] && !threat[ i ] && !isAround(myRow + dRow[i], myCol + dCol[i]) && !fallIntoNoEscape(myRow + dRow[i], myCol + dCol[i]) && isSafe(myRow + dRow[i], myCol + dCol[i])) {
            //int tmp2 = countNearbyEnemy(myRow + dRow[i], myCol + dCol[i], enemies);
            ii tmp = getMinDistanceToGold(myRow + dRow[i], myCol + dCol[i], enemies, i);
            int tmp1 = tmp.ff + 1;
            int tmp2 = tmp.ss + 1;
            moves.push_back( mp(mp(tmp1, tmp2), make_pair( visitTime[ myRow + dRow[ i ] ][ myCol + dCol[ i ] ], i )));
          }

        //also push DIG_LEFT and DIG_RIGHT
        if(canDigLeft && !isAround(myRow, myCol) && !fallIntoEnemy(myRow + 1, myCol - 1)) {
            //int tmp2 = countNearbyEnemy(myRow + 1, myCol - 1, enemies);
            ii tmp = getMinDistanceToGold(myRow + 1, myCol - 1, enemies, 5);
            int tmp1 = tmp.ff + 3;
            int tmp2 = tmp.ss + 3;
            moves.push_back( mp(mp(tmp1, tmp2), make_pair( visitTime[ myRow ][ myCol ], 5 )));
        }

        if(canDigRight && !isAround(myRow, myCol) && !fallIntoEnemy(myRow + 1, myCol + 1)) {
            //int tmp2 = countNearbyEnemy(myRow + 1, myCol + 1, enemies);
            ii tmp = getMinDistanceToGold(myRow + 1, myCol + 1, enemies, 6);
            int tmp1 = tmp.ff + 3;
            int tmp2 = tmp.ss + 3;
            moves.push_back( mp(mp(tmp1, tmp2), make_pair( visitTime[ myRow ][ myCol ], 6 )));
        }

        // 9/6/2013
        // or stay stunned
        if(!isAround(myRow, myCol) && isSafe(myRow, myCol)){
            //int tmp2 = countNearbyEnemy(myRow, myCol, enemies);
            ii tmp = getMinDistanceToGold(myRow, myCol, enemies, 0);
            int tmp1 = tmp.ff + 1;
            int tmp2 = tmp.ss + 1;
            moves.push_back( mp(mp(tmp1, tmp2), make_pair( visitTime[ myRow ][ myCol ], 0 )));
        }

        // If there are none, chose from among any other moves.
        // 5/6/2013: you should take no action if there's no good move.
        /*if ( moves.size() == 0 ) {
          for ( int i = 0; i < 4; i++ )
            if ( canMove[ i ] )
              moves.push_back( make_pair( visitTime[ myRow + dRow[ i ] ][ myCol + dCol[ i ] ], i ) );
        }*/

        if(moves.size() == 0) {
            if(!isRight && canEscape(myRow, myCol)) move = "RIGHT";
            else if(!isLeft && canEscape(myRow, myCol)) move = "LEFT";
            //if(!isRight) move = "RIGHT";
            //else if(!isLeft) move = "LEFT";
        }

        if ( moves.size() ) {
          // Randomly choose among equally good moves.
          //random_shuffle( moves.begin(), moves.end() );
          sort( moves.begin(), moves.end() );

          move = moveName[ moves.front().second.ss ];

        }
      }
    }

    // Send the chosen move to the game.
    cout << move << endl;
  }
}
