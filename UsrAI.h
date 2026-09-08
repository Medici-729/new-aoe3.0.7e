#ifndef USRAI_H
#define USRAI_H

#include "ai.h"
#include <unordered_map>

extern tagGame tagUsrGame;
extern ins UsrIns;
/*##########DO NOT MODIFY THE CODE ABOVE##########*/

class UsrAI:public AI
{
public:
    UsrAI(){this->id=0;}
    ~UsrAI(){}

private:
    void processData() override;
    int AddToIns(instruction ins) override
        {
            UsrIns.lock.lock();
            ins.id=UsrIns.g_id;
            UsrIns.g_id++;
            UsrIns.instructions.push(ins);
            UsrIns.lock.unlock();
            return ins.id;
        }
    tagInfo getInfo(){return tagUsrGame.getInfo();}
    void clearInsRet() override
    {
        tagUsrGame.clearInsRet();
    }
    /*##########DO NOT MODIFY THE CODE IN THE CLASS##########*/
    double  calDistance(double dr1, double ur1, double dr2, double ur2);
    double blockToDetail(int block);
    void updateTerrainCache(const tagInfo& info);
    bool findEmptyBlock(int& outDR, int& outUR, int size);
    void updateStage(const tagInfo& info);
    void cutTree(const tagInfo& info, int num, int resourceType);
    void hunting(const tagInfo& info, int targetCount);
    void buildBuilding(const tagInfo& info, int buildingType, int num);
    void armymanage(const tagInfo& info);
    void priestManage(const tagInfo& info);
    void priestFindway(const tagInfo& info, int priestSN, double priestDR, double priestUR);
};

#endif
