#include "UsrAI.h"
#include <cmath>
#include <cstdlib>
#include <climits>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
using namespace std;

tagGame tagUsrGame;
ins UsrIns;
/*##########DO NOT MODIFY THE CODE ABOVE##########*/
#define MAP_SIZE 100
#define stageExplore 1
#define stageDefense1 2
#define stageDefense2 3
#define stageAttack 4
static int stage=1;
static int terrainCache[MAP_SIZE][MAP_SIZE];

//距离计算函数
static double calDistance(double dr1,double ur1,double dr2,double ur2){
    double ddr=dr1-dr2;
    double dur=ur1-ur2;
    return sqrt(ddr*ddr+dur*dur);
}
//块坐标转换为细节坐标
static double blockToDetail(int block) {
    return block * BLOCKSIDELENGTH + BLOCKSIDELENGTH / 2.0;
}
//地图缓存更新
void UsrAI:: updateTerrainCache(const tagInfo& info) {
    for (int i=0;i<MAP_SIZE;i++){
        for(int j=0;j<MAP_SIZE;j++){ terrainCache[i][j]=-1;}// 初始化所有格子为 -1
    }
    if(info.theMap!=0){
        for(int i=0;i<MAP_SIZE;i++){
            for(int j=0;j<MAP_SIZE;j++){
        tagTerrain& t=(*info.theMap)[i][j];
        if(t.type==MAPPATTERN_GRASS&&t.height>=0){terrainCache[i][j]=0;}
            }
        }
    }// 标记空地(0)
    for(tagBuilding& b:info.buildings){
        int size = 2;
        switch (b.Type) {
            case BUILDING_HOME:      size = 2; break;
            case BUILDING_ARROWTOWER:size = 2; break;
            case BUILDING_FARM:      size = 3; break;
            case BUILDING_CENTER:    size = 3; break;
            case BUILDING_STOCK:     size = 3; break;
            case BUILDING_GRANARY:   size = 3; break;
            case BUILDING_ARMYCAMP:  size = 3; break;
            case BUILDING_RANGE:     size = 3; break;
            case BUILDING_STABLE:    size = 3; break;
            case BUILDING_MARKET:    size = 3; break;
            case BUILDING_COLLAGE:   size = 3; break;
            case BUILDING_SIEGE:     size = 3; break;
            default:                 size = 2; break;
            }
        for(int i=b.BlockDR;i<b.BlockDR+size&&i<MAP_SIZE;i++){
            for(int j=b.BlockUR;j<b.BlockUR+size&&j<MAP_SIZE;j++){
                terrainCache[i][j]=1;
            }
        }  
    }//标记建筑(1)
    for(tagResource& r:info.resources){
        if(r.BlockDR>=0&&r.BlockDR<MAP_SIZE&&r.BlockUR>=0&&r.BlockUR<MAP_SIZE)
        {terrainCache[r.BlockDR][r.BlockUR]=2;}
        }//标记资源(2)
    for(tagFarmer& f:info.farmers){
        if(f.BlockDR>=0&&f.BlockDR<MAP_SIZE&&f.BlockUR>=0&&f.BlockUR<MAP_SIZE){
         terrainCache[f.BlockDR][f.BlockUR]=3;   
        }
     }
    for(tagArmy& a:info.armies){
        if(a.BlockDR>=0&&a.BlockDR<MAP_SIZE&&a.BlockUR>=0&&a.BlockUR<MAP_SIZE){
         terrainCache[a.BlockDR][a.BlockUR]=3;   
        }
     }//标记单位(3)
}
//找空地
static bool findEmptyBlock(int& outDR, int& outUR, int size) {
    for (int i = 0; i <= MAP_SIZE - size; i++) {
        for (int j = 0; j <= MAP_SIZE - size; j++) {
            bool ok = true;
            for (int di = 0; di < size && ok; di++) {
                for (int dj = 0; dj < size && ok; dj++) {
                    if (terrainCache[i+di][j+dj] != 0) ok = false;
                }
            }
            if (ok) {
                outDR = i;
                outUR = j;
                return true;
            }
        }
    }
    return false;
}
//阶段切换
static void updateStage(tagInfo& info) {
    int frame=info.GameFrame;
    if(frame<6000) {
        stage=stageExplore;
    } else if(frame<13500) {
        stage=stageDefense1;
    } else if(frame<21000) {
        stage=stageDefense2;
    } else {
        stage=stageAttack;
    }
}
//采集：砍树，采浆果，采金矿，挖石头
void  UsrAI::cutTree(tagInfo& info,int num,int resourceType) {
    int count=0;
    for(tagFarmer& f:info.farmers){
        if(f.FarmerSort!=FARMERTYPE_FARMER) continue;
        if(f.Blood<=0) continue;
        if(f.NowState!=HUMAN_STATE_IDLE) continue;
        int targetSN=-1;
        double minDist=1e9;
        for(tagResource& r:info.resources){
            if(r.Type!=resourceType) continue;
            if(r.Blood<=0&&r.Cnt<=0) continue;
            double dist=calDistance(f.DR,f.UR,r.DR,r.UR);
            if(dist<minDist){
                minDist=dist;
                targetSN=r.SN;
            }
        }
        if(targetSN!=-1){
            HumanAction(f.SN,targetSN);
            count++;
        }
        if(count>=num) break;
    }    
}
//打猎：羚羊
void UsrAI:: hunting(tagInfo& info, int targetCount) {
    if (targetCount <= 0) return;
    int assigned = 0;
    vector<tagResource*> gazelles;
    for (tagResource& r : info.resources) {
        if (r.Type == RESOURCE_GAZELLE && (r.Blood > 0 || r.Cnt > 0)) {
            gazelles.push_back(&r);
        }
    }
    if (gazelles.empty()) return;
    for (tagFarmer& f : info.farmers) {
        if (f.FarmerSort != FARMERTYPE_FARMER) continue;
        if (f.Blood <= 0) continue;
        if (f.NowState != HUMAN_STATE_IDLE) continue;
        int targetSN = -1;
        double minDist = 1e9;
        for (tagResource* g : gazelles) {
            double d = calDistance(f.DR, f.UR, g->DR, g->UR);
            if (d < minDist) { minDist = d; targetSN = g->SN; }
        }
        if (targetSN == -1) return;
        HumanAction(f.SN, targetSN);
        assigned++;
        if (assigned >= targetCount) break;
    }
}
//建筑：市镇中心，谷仓，市场，农田，兵营、靶场 、马厩
void UsrAI:: buildBuilding(tagInfo& info,int buildingType,int num){
    int size=3;
    if(buildingType==BUILDING_HOME||buildingType==BUILDING_ARROWTOWER) size=2;
    int currentCount=0;
    int buildDR,buildUR;
    if(!findEmptyBlock(buildDR,buildUR,size))  return;
    for(tagFarmer& f:info.farmers){
        if(f.FarmerSort!=FARMERTYPE_FARMER) continue;
        if(f.Blood<=0) continue;
        if(f.NowState!=HUMAN_STATE_IDLE) continue;
        currentCount++;
        if(currentCount>num) break;
        HumanBuild(f.SN,buildingType,buildDR,buildUR);
     }
}
//军队管理
void UsrAI::armymanage(tagInfo& info){
    for(tagArmy& a:info.armies){
        if(a.Sort==AT_PRIEST) continue;
        if(a.Blood<=0) continue;
        if(a.NowState!=HUMAN_STATE_IDLE&&a.NowState!=HUMAN_STATE_WALKING) continue;
        int targetSN=-1;
        double minDist=1e9;
        for(tagArmy& enemy:info.enemy_armies){
            double d=calDistance(a.DR,a.UR,enemy.DR,enemy.UR);
            if (d > 15 * BLOCKSIDELENGTH) continue;
            if (enemy.Sort == AT_CHARIOT_ARCHER || enemy.Sort == AT_COMPOSITE_BOWMAN || enemy.Sort == AT_STONE_THROWER){
                 targetSN=enemy.SN;
                 break;  
           }
        }
        if(targetSN==-1&&!info.enemy_armies.empty()){
            for(tagArmy& enemy:info.enemy_armies){
                double d = calDistance(a.DR, a.UR, enemy.DR, enemy.UR);
                if (d < 15 * BLOCKSIDELENGTH && d < minDist) {
                    minDist = d;
                    targetSN = enemy.SN;
                 }
            }
        }
        if(targetSN==-1&&stage>=stageAttack&&!info.enemy_buildings.empty()){
            for(tagBuilding& enemy : info.enemy_buildings){
                double eDR = blockToDetail(enemy.BlockDR);
                double eUR = blockToDetail(enemy.BlockUR);
                double d = calDistance(a.DR, a.UR, eDR, eUR);
                if (d < 20 * BLOCKSIDELENGTH && d < minDist) {
                    minDist = d;
                    targetSN = enemy.SN;
                 }
            }
        }
        if(targetSN!=-1){
            HumanAction(a.SN,targetSN);
        }
    }
}
//祭司管理：转化敌人，躲避
void UsrAI::priestManage(tagInfo& info){
    int priestSN=-1;
    double priestDR=0,priestUR=0;
    int convertCooldown=0;
    for(tagArmy& a:info.armies){
        if(a.Sort==AT_PRIEST&&a.Blood>0){
            priestSN=a.SN;
            priestDR=a.DR;
            priestUR=a.UR;
            convertCooldown=a.ConvertCooldown;
            break;
        }
    }
    if(priestSN==-1) return;
    bool enemyDetected=false;
    double enemyDR=0,enemyUR=0;
    int enemySN=-1;
    for(tagArmy& e:info.enemy_armies){
        double d=calDistance(priestDR,priestUR,e.DR,e.UR);
        if(d<15*BLOCKSIDELENGTH){
            enemyDetected=true;
            enemyDR=e.DR;
            enemyUR=e.UR;
            enemySN=e.SN;
        }
    }
    if (enemyDetected) {
        double towerDR = -1, towerUR = -1;
        double minTowerDist = 1e9;
        for (tagBuilding& b : info.buildings) {
            if (b.Type == BUILDING_ARROWTOWER && b.Percent == 100) {
                double bDR = blockToDetail(b.BlockDR);
                double bUR = blockToDetail(b.BlockUR);
                double d = calDistance(priestDR, priestUR, bDR, bUR);
                if (d < minTowerDist) {
                    minTowerDist = d;
                    towerDR = bDR;
                    towerUR = bUR;
                }
            }
        }
        if (towerDR != -1) {
            double targetDR = towerDR - 1 * BLOCKSIDELENGTH;
            double targetUR = towerUR - 1 * BLOCKSIDELENGTH;
            if (minTowerDist > 3 * BLOCKSIDELENGTH) {
                HumanMove(priestSN, targetDR, targetUR);
            } else {
                if (convertCooldown == 0 && enemySN != -1) {
                    HumanAction(priestSN, enemySN);
                }
            }
        }
        return;
    }
    if(stage==stageExplore){
        priestFindway(info,priestSN,priestDR,priestUR);
    }
}
//祭司探路
static double lastDR = -1, lastUR = -1;
static int stuckFrames = 0;
static int step = 0;
static double targetDR = -1, targetUR = -1;
static void priestFindway(tagInfo& info,int priestSN,double priestDR,double priestUR){
    int centerDR = -1, centerUR = -1;
    for (tagBuilding& b : info.buildings) {
        if (b.Type == BUILDING_CENTER) {
            centerDR = b.BlockDR;
            centerUR = b.BlockUR;
            break;
        }
    }
    if (centerDR == -1) return;
    if (lastDR != -1 && lastUR != -1) {
        double moved = calDistance(lastDR, lastUR, priestDR, priestUR);
        if (moved < 0.3 * BLOCKSIDELENGTH) {
            stuckFrames++;
        } else {
            stuckFrames = 0;
        }
    }
    lastDR = priestDR;
    lastUR = priestUR;//检测是否卡住
    bool reached = false;
    if (targetDR != -1 && targetUR != -1) {
        double d = calDistance(priestDR, priestUR, targetDR, targetUR);
        if (d < 2 * BLOCKSIDELENGTH) reached = true;
    }
    if (stuckFrames > 5 || reached) {
        step = (step + 1) % 4;
        targetDR = -1;
        targetUR = -1;
        stuckFrames = 0;
    }
    if (targetDR == -1 && targetUR == -1) {
        int dist = 25;
        switch (step) {
            case 0:
                targetDR = blockToDetail(max(0, centerDR - dist));
                targetUR = blockToDetail(centerUR);
                break;
            case 1:
                targetDR = blockToDetail(min(MAP_SIZE - 1, centerDR + dist));
                targetUR = blockToDetail(centerUR);
                break;
            case 2:
                targetDR = blockToDetail(centerDR);
                targetUR = blockToDetail(max(0, centerUR - dist));
                break;
            case 3:
                targetDR = blockToDetail(centerDR);
                targetUR = blockToDetail(min(MAP_SIZE - 1, centerUR + dist));
                break;
        }
        int bDR = (int)(targetDR / BLOCKSIDELENGTH);
        int bUR = (int)(targetUR / BLOCKSIDELENGTH);
        if (bDR >= 0 && bDR < MAP_SIZE && bUR >= 0 && bUR < MAP_SIZE) {
            if (terrainCache[bDR][bUR] != 0) {
                step = (step + 1) % 4;
                targetDR = -1;
                targetUR = -1;
                return;
            }
        }
    }
    if (targetDR != -1 && targetUR != -1) {
        HumanMove(priestSN, targetDR, targetUR);
    }
}

/* ============================== 主入口 ============================== */
void UsrAI::processData ()
{
     tagInfo info = getInfo();
     if (info.GameFrame % 5 != 0) return;
     updateTerrainCache(info);
     updateStage(info);
     priestManage(info);
     int woodcutNum=2;
     int berrypickNum=2;
     int huntNum=2;
     int buildNum=1;
     int stonedigNum=1;
     cutTree(info,woodcutNum,RESOURCE_WOOD);
     hunting(info, huntNum);


}
