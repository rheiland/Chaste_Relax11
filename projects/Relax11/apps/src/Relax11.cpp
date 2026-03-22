/*
 * Relax11: 11 cells placed along the x-axis with overlapping radii.
 * Pure repulsion, no growth, no division.
 *
 * Layout
 * ------
 * Cell ID 0 is on the far left; ID 10 on the far right.
 * Middle cell (ID 5) is at the origin.
 * radius   = 5
 * overlap  = 5  →  centre-to-centre spacing = 2*r - overlap = 5
 * Position of cell i: x = (i - 5) * 5,  y = 0
 */

#include "CheckpointArchiveTypes.hpp"
#include "ExecutableSupport.hpp"
#include "Exception.hpp"

#include <cmath>
#include <fstream>
#include <string>
#include <vector>

#include "SimulationTime.hpp"

// Mesh & population
#include "NodesOnlyMesh.hpp"
#include "NodeBasedCellPopulation.hpp"

// Simulation
#include "OffLatticeSimulation.hpp"

// Force
#include "RepulsionForce.hpp"

// Cell infrastructure
#include "AbstractCellCycleModel.hpp"
#include "WildTypeCellMutationState.hpp"
#include "DifferentiatedCellProliferativeType.hpp"

// Modifier infrastructure
#include "AbstractCellBasedSimulationModifier.hpp"

// Utilities
#include "SmartPointers.hpp"
#include "OutputFileHandler.hpp"

// ============================================================
// Cell-cycle model that never triggers division
// ============================================================
class NeverDivideCellCycleModel : public AbstractCellCycleModel
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
        ar & boost::serialization::base_object<AbstractCellCycleModel>(*this);
    }

public:
    NeverDivideCellCycleModel() : AbstractCellCycleModel() {}
    NeverDivideCellCycleModel(const NeverDivideCellCycleModel& rModel)
        : AbstractCellCycleModel(rModel) {}

    bool ReadyToDivide() override { return false; }

    AbstractCellCycleModel* CreateCellCycleModel() override
    {
        return new NeverDivideCellCycleModel(*this);
    }

    double GetAverageTransitCellCycleTime() override { return 1e9; }
    double GetAverageStemCellCycleTime()    override { return 1e9; }

    void OutputCellCycleModelParameters(out_stream& rParamsFile) override
    {
        AbstractCellCycleModel::OutputCellCycleModelParameters(rParamsFile);
    }
};

// ============================================================
// Modifier: write CSV and debug output every timestep
// ============================================================
class CsvOutputModifier : public AbstractCellBasedSimulationModifier<2, 2>
{
private:
    std::ofstream mCsvFile;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
        ar & boost::serialization::base_object<
                AbstractCellBasedSimulationModifier<2,2> >(*this);
    }

public:
    CsvOutputModifier() : AbstractCellBasedSimulationModifier<2,2>() {}

    void UpdateAtEndOfTimeStep(AbstractCellPopulation<2,2>& rPop) override
    {
        double t = SimulationTime::Instance()->GetTime();
        std::cout << "[DEBUG] t=" << t
                  << "  ncells=" << rPop.GetNumRealCells() << "\n";
        for (auto iter = rPop.Begin(); iter != rPop.End(); ++iter)
        {
            unsigned idx = rPop.GetLocationIndexUsingCell(*iter);
            const c_vector<double, 2>& pos = rPop.GetNode(idx)->rGetLocation();
            double radius = rPop.GetNode(idx)->GetRadius();
            std::cout << "  cell " << idx
                      << "  x=" << pos[0] << "  y=" << pos[1] << "\n";
            if (mCsvFile.is_open())
            {
                mCsvFile << t << "," << idx << ","
                         << pos[0] << "," << pos[1] << "," << radius << "\n";
            }
        }
    }

    void SetupSolve(AbstractCellPopulation<2,2>& rPop,
                    std::string outputDirectory) override
    {
        OutputFileHandler handler(outputDirectory, false);
        mCsvFile.open(handler.GetOutputDirectoryFullPath() + "cells.csv");
        mCsvFile << "time,cell_id,x,y,radius\n";
        UpdateAtEndOfTimeStep(rPop);
    }

    void OutputSimulationModifierParameters(out_stream& rParamsFile) override
    {
        AbstractCellBasedSimulationModifier<2,2>::OutputSimulationModifierParameters(rParamsFile);
    }
};

// ============================================================
// main
// ============================================================
int main(int argc, char* argv[])
{
    ExecutableSupport::StandardStartup(&argc, &argv);
    SimulationTime::Instance()->SetStartTime(0.0);

    int exit_code = ExecutableSupport::EXIT_OK;
    try
    {
        const int    N_CELLS  = 11;
        const double RADIUS   = 5.0;
        const double OVERLAP  = 5.0;
        const double SPACING  = 2.0 * RADIUS - OVERLAP;  // = 5.0

        const double dt       = 0.01;
        const double end_time = 5.0;

        // Cutoff must exceed 2 * radius
        const double interaction_radius = 2.0 * RADIUS + 1.0;  // = 11.0

        // ---- Nodes ----
        // Cell ID 0 on the far left; ID 5 at origin; ID 10 on the far right
        std::vector<Node<2>*> nodes;
        for (int i = 0; i < N_CELLS; ++i)
        {
            double x = (i - 5) * SPACING;
            nodes.push_back(new Node<2>(i, false, x, 0.0));
        }

        NodesOnlyMesh<2> mesh;
        mesh.ConstructNodesWithoutMesh(nodes, interaction_radius);
        for (auto* n : nodes) { delete n; }

        // ---- Cells ----
        MAKE_PTR(WildTypeCellMutationState,    p_state);
        MAKE_PTR(DifferentiatedCellProliferativeType, p_type);

        std::vector<CellPtr> cells;
        for (int i = 0; i < N_CELLS; ++i)
        {
            auto* p_model = new NeverDivideCellCycleModel();
            CellPtr p_cell(new Cell(p_state, p_model));
            p_cell->SetCellProliferativeType(p_type);
            p_cell->InitialiseCellCycleModel();
            p_cell->GetCellData()->SetItem("Radius", RADIUS);
            cells.push_back(p_cell);
        }

        // ---- Population ----
        NodeBasedCellPopulation<2> population(mesh, cells);
        population.SetUseVariableRadii(true);
        for (int i = 0; i < N_CELLS; ++i)
        {
            population.GetNode(i)->SetRadius(RADIUS);
        }

        // ---- Simulator ----
        OffLatticeSimulation<2> sim(population);
        sim.SetOutputDirectory("Relax11");
        sim.SetDt(dt);
        sim.SetSamplingTimestepMultiple(10);
        sim.SetEndTime(end_time);

        // ---- Pure repulsion ----
        MAKE_PTR(RepulsionForce<2>, p_force);
        sim.AddForce(p_force);

        // ---- CSV + debug output ----
        boost::shared_ptr<CsvOutputModifier> p_modifier(new CsvOutputModifier());
        sim.AddSimulationModifier(p_modifier);

        sim.Solve();
    }
    catch (const Exception& e)
    {
        ExecutableSupport::PrintError(e.GetMessage());
        exit_code = ExecutableSupport::EXIT_ERROR;
    }

    ExecutableSupport::FinalizePetsc();
    return exit_code;
}
