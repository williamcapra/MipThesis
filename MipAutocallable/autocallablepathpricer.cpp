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


Real AutocallablePathPricer::operator()(const Path& path) const {

	Calendar calendar = TARGET();
	DayCounter dayCount = Actual365Fixed();

	//evaluation times
	Time firstevaluation1 = dayCount.yearFraction(settlementDate_, Date(21, February, 2018));
	Time firstevaluation2 = dayCount.yearFraction(settlementDate_, Date(22, February, 2018));
	Time firstevaluation3 = dayCount.yearFraction(settlementDate_, Date(23, February, 2018));
	Time firstevaluation4 = dayCount.yearFraction(settlementDate_, Date(26, February, 2018));
	Time firstevaluation5 = dayCount.yearFraction(settlementDate_, Date(27, February, 2018));
	Time secondevaluation1 = dayCount.yearFraction(settlementDate_, Date(20, February, 2019));
	Time secondevaluation2 = dayCount.yearFraction(settlementDate_, Date(21, February, 2019));
	Time secondevaluation3 = dayCount.yearFraction(settlementDate_, Date(22, February, 2019));
	Time secondevaluation4 = dayCount.yearFraction(settlementDate_, Date(25, February, 2019));
	Time secondevaluation5 = dayCount.yearFraction(settlementDate_, Date(26, February, 2019));
	Time thirdevaluation1 = dayCount.yearFraction(settlementDate_, Date(20, February, 2020));
	Time thirdevaluation2 = dayCount.yearFraction(settlementDate_, Date(21, February, 2020));
	Time thirdevaluation3 = dayCount.yearFraction(settlementDate_, Date(24, February, 2020));
	Time thirdevaluation4 = dayCount.yearFraction(settlementDate_, Date(25, February, 2020));
	Time thirdevaluation5 = dayCount.yearFraction(settlementDate_, Date(26, February, 2020));
	Time barrierevaluation = dayCount.yearFraction(settlementDate_, Date(01, March, 2021));

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

	//payment dates
	Date firstpaymentdate(05, March, 2018);
	Date secondpaymentdate(04, March, 2019);
	Date thirdpaymentdate(03, March, 2020);

	//bond settings
	Real faceAmount = 1000.0;
	boost::shared_ptr<PricingEngine> bondEngine(new DiscountingBondEngine(Handle<YieldTermStructure>(bondTermStructure_)));
	ZeroCouponBond firstZeroCouponBond(2, calendar,	faceAmount,	firstpaymentdate, Following, Real(100.0), settlementDate_);
	firstZeroCouponBond.setPricingEngine(bondEngine);
	ZeroCouponBond secondZeroCouponBond(2, calendar, faceAmount, secondpaymentdate, Following, Real(100.0), settlementDate_);
	secondZeroCouponBond.setPricingEngine(bondEngine);
	ZeroCouponBond thirdZeroCouponBond(2, calendar, faceAmount, thirdpaymentdate, Following, Real(100.0), settlementDate_);
	thirdZeroCouponBond.setPricingEngine(bondEngine);
	ZeroCouponBond barrierZeroCouponBond(2, calendar, faceAmount, Date(03, March, 2021), Following, Real(100.0), settlementDate_);
	thirdZeroCouponBond.setPricingEngine(bondEngine);
	
	//MC initialization
	Real price = 0.00;

	if (path.value(firstevaluation1) >= firstlevel || path.value(firstevaluation2) >= firstlevel || path.value(firstevaluation3) >= firstlevel
		|| path.value(firstevaluation4) >= firstlevel || path.value(firstevaluation5) >= firstlevel) 
	{
		price = firstZeroCouponBond.NPV() + firstplus * OISTermStructure_->discount(firstpaymentdate);
	}
	else if (path.value(secondevaluation1) >= secondlevel || path.value(secondevaluation2) >= secondlevel || path.value(secondevaluation3) >= secondlevel
		|| path.value(secondevaluation4) >= secondlevel || path.value(secondevaluation5) >= secondlevel)
	{
		price = secondZeroCouponBond.NPV() + secondplus * OISTermStructure_->discount(secondpaymentdate);
	}
	else if (path.value(thirdevaluation1) >= thirdlevel || path.value(thirdevaluation2) >= thirdlevel || path.value(thirdevaluation3) >= thirdlevel
		|| path.value(thirdevaluation4) >= thirdlevel || path.value(thirdevaluation5) >= thirdlevel)
	{
		price = thirdZeroCouponBond.NPV() + thirdplus * OISTermStructure_->discount(thirdpaymentdate);
	}
	else if (path.value(barrierevaluation) >= barrierlevel) 
	{
		price = barrierZeroCouponBond.NPV() + barrierplus * OISTermStructure_->discount(maturity_);
	}
	else {
		price = barrierZeroCouponBond.NPV() - (path.value(barrierevaluation) / startinglevel -1)* OISTermStructure_->discount(maturity_);
	}
		
	return price;
}