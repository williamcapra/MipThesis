#include <ql/quantlib.hpp>
#include <autocallablesimulation.hpp>
#include <autocallablepathpricer.hpp>

using namespace QuantLib;

AutocallableSimulation::AutocallableSimulation(boost::shared_ptr<Quote> underlying,	
	boost::shared_ptr<YieldTermStructure> qTermStructure,
	boost::shared_ptr<YieldTermStructure> bondTermStructure,
	boost::shared_ptr<YieldTermStructure> OISTermStructure,
	boost::shared_ptr<BlackVolTermStructure> volatility,
	Time maturity,
	Real strike,
	Date settlementDate)
	: underlying_(underlying), qTermStructure_(qTermStructure),	bondTermStructure_(bondTermStructure),
	OISTermStructure_(OISTermStructure), volatility_(volatility), maturity_(maturity), strike_(strike), settlementDate_(settlementDate){
}


void AutocallableSimulation::compute(Size nTimeSteps, Size nSamples) {
	
	//diffusion: it sets the process of underlying diffusion
	boost::shared_ptr<StochasticProcess1D> diffusion(new BlackScholesMertonProcess(
		Handle<Quote>(underlying_),
		Handle<YieldTermStructure>(qTermStructure_),
		Handle<YieldTermStructure>(OISTermStructure_),
		Handle<BlackVolTermStructure>(volatility_))
	);
	//generator: it generates the pseudo-random numbers
	PseudoRandom::rsg_type rsg = PseudoRandom::make_sequence_generator(nTimeSteps, 0);
	bool brownianBridge = false;
	typedef SingleVariate<PseudoRandom>::path_generator_type generator_type;
	boost::shared_ptr<generator_type> myPathGenerator(new
		generator_type(diffusion, maturity_, nTimeSteps,
			rsg, brownianBridge));

	boost::shared_ptr<PathPricer<Path>> myPathPricer(
		new AutocallablePathPricer(bondTermStructure_,
			OISTermStructure_,
			maturity_, 
			strike_,
			settlementDate_));

	// staticsticAccumulator: it is necessary to calculate the average over the path-prices
	Statistics statisticsAccumulator;

	// The Monte Carlo model generates paths using myPathGenerator
	// each path is priced using myPathPricer
	// prices will be accumulated into statisticsAccumulator

	MonteCarloModel<SingleVariate, PseudoRandom>
		MCSimulation(myPathGenerator,
			myPathPricer,
			statisticsAccumulator,
			false);

	// the model simulates nSamples paths
	MCSimulation.addSamples(nSamples);

	// the sampleAccumulator method
	// gives access to all the methods of statisticsAccumulator
	Real Price = MCSimulation.sampleAccumulator().mean();
	std::cout << " \nPrice = " << Price << std::endl;
}