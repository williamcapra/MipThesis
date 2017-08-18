#pragma once
#ifndef autocallable_path_pricer_hpp
#define autocallable_path_pricer_hpp

#include <ql/quantlib.hpp>

using namespace QuantLib;

// The key for the MonteCarlo simulation is to have a PathPricer that
// implements a value(const Path& path) method.

class AutocallablePathPricer : public PathPricer<Path> {
public:
	// real constructor
	AutocallablePathPricer(boost::shared_ptr<YieldTermStructure> bondTermStructure,
		boost::shared_ptr<YieldTermStructure> OISTermStructure,
		Time maturity,
		Real strike,
		Date settlementDate);

	// The value() method encapsulates the pricing code
	Real operator()(const Path& path) const;

private:
	boost::shared_ptr<YieldTermStructure> bondTermStructure_;
	boost::shared_ptr<YieldTermStructure> OISTermStructure_;
	Time maturity_;
	Real strike_;
	Date settlementDate_;
};


#endif // !autocallable_path_pricer_h
