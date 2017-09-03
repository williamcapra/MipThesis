#include <ql/quantlib.hpp>
#include <autocallablepathpricer.hpp>

using namespace QuantLib;

// real constructor
AutocallablePathPricer::AutocallablePathPricer(boost::shared_ptr<YieldTermStructure> bondTermStructure,
	boost::shared_ptr<YieldTermStructure> OISTermStructure,
	Time maturity,
	Real strike,
	Date settlementDate)
	: bondTermStructure_(bondTermStructure), OISTermStructure_(OISTermStructure), maturity_(maturity), strike_(strike), settlementDate_(settlementDate) {
	QL_REQUIRE(maturity_ > 0.0, "maturity must be positive");
	QL_REQUIRE(strike_ > 0.0, "strike must be positive");
}

Real AutocallablePathPricer::operator()(const MultiPath& paths) const {

	const Path& path = paths[0];
	//const Path& vol = paths[1];

	Calendar calendar = TARGET();
	DayCounter dayCount = ActualActual();

	//evaluation times
	Time first_evaluationStart = dayCount.yearFraction(settlementDate_, Date(21, February, 2018));
	Time first_evaluationEnd = dayCount.yearFraction(settlementDate_, Date(27, February, 2018));
	Time second_evaluationStart = dayCount.yearFraction(settlementDate_, Date(20, February, 2019));
	Time second_evaluationEnd = dayCount.yearFraction(settlementDate_, Date(26, February, 2019));
	Time third_evaluationStart = dayCount.yearFraction(settlementDate_, Date(20, February, 2020));
	Time third_evaluationEnd = dayCount.yearFraction(settlementDate_, Date(26, February, 2020));
	Time barrier_evaluation = dayCount.yearFraction(settlementDate_, Date(01, March, 2021));
	Time time_tolerance = 0.00001;

	//autocallable and barrier levels
	Real startinglevel = strike_;
	Real firstlevel = (100 * startinglevel) / 100;
	Real secondlevel = (100 * startinglevel) / 100;
	Real thirdlevel = (100 * startinglevel) / 100;
	Real barrierlevel = (60 * startinglevel) / 100;

	//payment ammount, multiplied by 0, 1 or 2 for the first, the second and the third period respectivelly
	Real firstplus = 58 * 0;
	Real secondplus = 58 * 1;
	Real thirdplus = 58 * 2;
	Real barrierplus = 58 * 3;
	Real multiplo = 66.2954 / 100;

	//payment dates
	Date firstpaymentdate(05, March, 2018);
	Date secondpaymentdate(04, March, 2019);
	Date thirdpaymentdate(03, March, 2020);

	//bond settings
	Real faceAmount = 1000.0;
	boost::shared_ptr<PricingEngine> bondEngine(new DiscountingBondEngine(Handle<YieldTermStructure>(bondTermStructure_)));
	ZeroCouponBond firstZeroCouponBond(2, calendar, faceAmount, firstpaymentdate, Following, Real(100.0), settlementDate_);
	firstZeroCouponBond.setPricingEngine(bondEngine);
	ZeroCouponBond secondZeroCouponBond(2, calendar, faceAmount, secondpaymentdate, Following, Real(100.0), settlementDate_);
	secondZeroCouponBond.setPricingEngine(bondEngine);
	ZeroCouponBond thirdZeroCouponBond(2, calendar, faceAmount, thirdpaymentdate, Following, Real(100.0), settlementDate_);
	thirdZeroCouponBond.setPricingEngine(bondEngine);
	ZeroCouponBond barrierZeroCouponBond(2, calendar, faceAmount, Date(03, March, 2021), Following, Real(100.0), settlementDate_);
	barrierZeroCouponBond.setPricingEngine(bondEngine);

	//Pricing algorithm
	Size n = path.length() - 1;
	QL_REQUIRE(n > 0, "the path cannot be empty");
	Time dt = maturity_ / n;
	Time t = 0;
	Real stock = path.front();
	
	for (Size step = 0; step < n - 1; step ++) {
		t += dt;
		stock = path[step+1];
		if (t > (first_evaluationStart - time_tolerance) && t < (first_evaluationEnd + time_tolerance)) {
			if (stock >= firstlevel) {
				return (firstZeroCouponBond.NPV() + firstplus * OISTermStructure_->discount(firstpaymentdate));				
			}		
		}
		if (t > (second_evaluationStart - time_tolerance) && t < (second_evaluationEnd + time_tolerance)) {
			if (stock >= secondlevel) {
				return (secondZeroCouponBond.NPV() + secondplus * OISTermStructure_->discount(secondpaymentdate));
			}
		}
		if (t > (third_evaluationStart - time_tolerance) && t < (third_evaluationEnd + time_tolerance)) {
			if (stock >= thirdlevel) {
				return (thirdZeroCouponBond.NPV() + thirdplus * OISTermStructure_->discount(thirdpaymentdate));
			}
		}
		if ( t > (barrier_evaluation - time_tolerance) && t < (barrier_evaluation + time_tolerance)) {
			if (stock > barrierlevel) {
				return (barrierZeroCouponBond.NPV() + barrierplus * OISTermStructure_->discount(maturity_));
			}
			else {
				Real performance = (1 - stock / startinglevel) * multiplo;
				return (barrierZeroCouponBond.NPV() * (1 - performance));
			}
		}
	}
}