#include "A4Header.h"

using namespace SVF;
using namespace llvm;
using namespace std;

int main(int argc, char **argv)
{
    auto moduleNameVec =
            OptionBase::parseOptions(argc, argv, "Whole Program Points-to Analysis",
                                     "[options] <input-bitcode...>");

    LLVMModuleSet::buildSVFModule(moduleNameVec);

    SVFIRBuilder builder;
    auto pag = builder.build();
    pag->dump("PAG");

    CFLR solver;
    solver.buildGraph(pag);
    solver.solve();
    solver.dumpResult();

    LLVMModuleSet::releaseLLVMModuleSet();
    return 0;
}


void CFLR::solve()
{
    std::unordered_set<unsigned> allNodes;
    
    for (auto &nodeItr : graph->getSuccessorMap())
    {
        unsigned src = nodeItr.first;
        allNodes.insert(src);
        
        for (auto &lblItr : nodeItr.second)
        {
            EdgeLabel label = lblItr.first;
            for (auto dst : lblItr.second)
            {
                allNodes.insert(dst);
                workList.push(CFLREdge(src, dst, label));
            }
        }
    }
    
    auto addEdge = [this](unsigned src, unsigned dst, EdgeLabel label)
    {
        if (!graph->hasEdge(src, dst, label)) {
            graph->addEdge(src, dst, label);
            workList.push(CFLREdge(src, dst, label));
        }
    };
    
    for (auto node : allNodes)
    {
        addEdge(node, node, VF);
        addEdge(node, node, VFBar);
        addEdge(node, node, VA);
    }
    
    while (!workList.empty())
    {
        CFLREdge edge = workList.pop();
        unsigned x = edge.src;
        unsigned z = edge.dst;
        EdgeLabel label = edge.label;
        
        auto &succMap = graph->getSuccessorMap();
        auto &predMap = graph->getPredecessorMap();
        
        if (label == VFBar && succMap.count(z) && succMap[z].count(AddrBar))
        {
            for (auto w : succMap[z][AddrBar])
                addEdge(x, w, PT);
        }
        if (label == AddrBar && predMap.count(x) && predMap[x].count(VFBar))
        {
            for (auto y : predMap[x][VFBar])
                addEdge(y, z, PT);
        }
        
        if (label == Addr && succMap.count(z) && succMap[z].count(VF))
        {
            for (auto w : succMap[z][VF])
                addEdge(x, w, PTBar);
        }
        if (label == VF && predMap.count(x) && predMap[x].count(Addr))
        {
            for (auto y : predMap[x][Addr])
                addEdge(y, z, PTBar);
        }
        
        if (label == VF)
        {
            if (succMap.count(z) && succMap[z].count(VF))
            {
                for (auto w : succMap[z][VF])
                    addEdge(x, w, VF);
            }
            if (predMap.count(x) && predMap[x].count(VF))
            {
                for (auto y : predMap[x][VF])
                    addEdge(y, z, VF);
            }
        }
        
        if (label == Copy)
        {
            addEdge(x, z, VF);
        }
        
        if (label == SV && succMap.count(z) && succMap[z].count(Load))
        {
            for (auto w : succMap[z][Load])
                addEdge(x, w, VF);
        }
        if (label == Load && predMap.count(x) && predMap[x].count(SV))
        {
            for (auto y : predMap[x][SV])
                addEdge(y, z, VF);
        }
        
        if (label == PV && succMap.count(z) && succMap[z].count(Load))
        {
            for (auto w : succMap[z][Load])
                addEdge(x, w, VF);
        }
        if (label == Load && predMap.count(x) && predMap[x].count(PV))
        {
            for (auto y : predMap[x][PV])
                addEdge(y, z, VF);
        }
        
        if (label == Store && succMap.count(z) && succMap[z].count(VP))
        {
            for (auto w : succMap[z][VP])
                addEdge(x, w, VF);
        }
        if (label == VP && predMap.count(x) && predMap[x].count(Store))
        {
            for (auto y : predMap[x][Store])
                addEdge(y, z, VF);
        }
        
        if (label == VFBar)
        {
            if (succMap.count(z) && succMap[z].count(VFBar))
            {
                for (auto w : succMap[z][VFBar])
                    addEdge(x, w, VFBar);
            }
            if (predMap.count(x) && predMap[x].count(VFBar))
            {
                for (auto y : predMap[x][VFBar])
                    addEdge(y, z, VFBar);
            }
        }
        
        if (label == CopyBar)
        {
            addEdge(x, z, VFBar);
        }
        
        if (label == LoadBar && succMap.count(z) && succMap[z].count(SVBar))
        {
            for (auto w : succMap[z][SVBar])
                addEdge(x, w, VFBar);
        }
        if (label == SVBar && predMap.count(x) && predMap[x].count(LoadBar))
        {
            for (auto y : predMap[x][LoadBar])
                addEdge(y, z, VFBar);
        }
        
        if (label == LoadBar && succMap.count(z) && succMap[z].count(VP))
        {
            for (auto w : succMap[z][VP])
                addEdge(x, w, VFBar);
        }
        if (label == VP && predMap.count(x) && predMap[x].count(LoadBar))
        {
            for (auto y : predMap[x][LoadBar])
                addEdge(y, z, VFBar);
        }
        
        if (label == PV && succMap.count(z) && succMap[z].count(StoreBar))
        {
            for (auto w : succMap[z][StoreBar])
                addEdge(x, w, VFBar);
        }
        if (label == StoreBar && predMap.count(x) && predMap[x].count(PV))
        {
            for (auto y : predMap[x][PV])
                addEdge(y, z, VFBar);
        }
        
        if (label == LV && succMap.count(z) && succMap[z].count(Load))
        {
            for (auto w : succMap[z][Load])
                addEdge(x, w, VA);
        }
        if (label == Load && predMap.count(x) && predMap[x].count(LV))
        {
            for (auto y : predMap[x][LV])
                addEdge(y, z, VA);
        }
        
        if (label == VFBar && succMap.count(z) && succMap[z].count(VA))
        {
            for (auto w : succMap[z][VA])
                addEdge(x, w, VA);
        }
        if (label == VA && predMap.count(x) && predMap[x].count(VFBar))
        {
            for (auto y : predMap[x][VFBar])
                addEdge(y, z, VA);
        }
        
        if (label == VA && succMap.count(z) && succMap[z].count(VF))
        {
            for (auto w : succMap[z][VF])
                addEdge(x, w, VA);
        }
        if (label == VF && predMap.count(x) && predMap[x].count(VA))
        {
            for (auto y : predMap[x][VA])
                addEdge(y, z, VA);
        }
        
        if (label == Store && succMap.count(z) && succMap[z].count(VA))
        {
            for (auto w : succMap[z][VA])
                addEdge(x, w, SV);
        }
        if (label == VA && predMap.count(x) && predMap[x].count(Store))
        {
            for (auto y : predMap[x][Store])
                addEdge(y, z, SV);
        }
        
        if (label == VA && succMap.count(z) && succMap[z].count(StoreBar))
        {
            for (auto w : succMap[z][StoreBar])
                addEdge(x, w, SVBar);
        }
        if (label == StoreBar && predMap.count(x) && predMap[x].count(VA))
        {
            for (auto y : predMap[x][VA])
                addEdge(y, z, SVBar);
        }
        
        if (label == PTBar && succMap.count(z) && succMap[z].count(VA))
        {
            for (auto w : succMap[z][VA])
                addEdge(x, w, PV);
        }
        if (label == VA && predMap.count(x) && predMap[x].count(PTBar))
        {
            for (auto y : predMap[x][PTBar])
                addEdge(y, z, PV);
        }
        
        if (label == VA && succMap.count(z) && succMap[z].count(PT))
        {
            for (auto w : succMap[z][PT])
                addEdge(x, w, VP);
        }
        if (label == PT && predMap.count(x) && predMap[x].count(VA))
        {
            for (auto y : predMap[x][VA])
                addEdge(y, z, VP);
        }
        
        if (label == LoadBar && succMap.count(z) && succMap[z].count(VA))
        {
            for (auto w : succMap[z][VA])
                addEdge(x, w, LV);
        }
        if (label == VA && predMap.count(x) && predMap[x].count(LoadBar))
        {
            for (auto y : predMap[x][LoadBar])
                addEdge(y, z, LV);
        }
    }
}
