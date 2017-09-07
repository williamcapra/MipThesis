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
	Time first_evaluation2 = dayCount.yearFraction(settlementDate_, Date(22, February, 2018));
	Time first_evaluation3 = dayCount.yearFraction(settlementDate_, Date(23, February, 2018));
	Time first_evaluation4 = dayCount.yearFraction(settlementDate_, Date(26, February, 2018));
	Time first_evaluationEnd = dayCount.yearFraction(settlementDate_, Date(27, February, 2018));

	Time second_evaluationStart = dayCount.yearFraction(settlementDate_, Date(20, February, 2019));
	Time second_evaluation2 = dayCount.yearFraction(settlementDate_, Date(21, February, 2019));
	Time second_evaluation3 = dayCount.yearFraction(settlementDate_, Date(22, February, 2019));
	Time second_evaluation4 = dayCount.yearFraction(settlementDate_, Date(25, February, 2019));
	Time second_evaluationEnd = dayCount.yearFraction(settlementDate_, Date(26, February, 2019));

	Time third_evaluationStart = dayCount.yearFraction(settlementDate_, Date(20, February, 2020));
	Time third_evaluation2 = dayCount.yearFraction(settlementDate_, Date(21, February, 2020));
	Time third_evaluation3 = dayCount.yearFraction(settlementDate_, Date(24, February, 2020));
	Time third_evaluation4 = dayCount.yearFraction(settlementDate_, Date(25, February, 2020));
	Time third_evaluationEnd = dayCount.yearFraction(settlementDate_, Date(26, February, 2020));

	Time barrier_evaluation = dayCount.yearFraction(settlementDate_, Date(01, March, 2021));

	//autocallable and barrier levels
	Real startinglevel = strike_;
	Real excerciselevel = 15.08;
	Real barrierlevel = 9.0504;

	Real plus = 58;

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
	Size n = path.length()-1;
	QL_REQUIRE(n > 0, "the path cannot be empty");
	Time dt = maturity_ / n;

	//initialization
	Time t = 0;
	Real price = 0;
	Real stock = 0;
	Real performance = 0;
	bool alive = true;
	int step = 0;

	while ((alive) && (t < barrier_evaluation)) {
		
		stock = path[step];

		if ((t >= first_evaluationStart) && (t <= first_evaluation3) && (stock >= excerciselevel)) {
			price = plus * OISTermStructure_->discount(firstpaymentdate) + firstZeroCouponBond.NPV();
			alive = false;
		} else if ((t >= first_evaluation4) && (t <= first_evaluationEnd) && (stock >= excerciselevel)) {
			price = plus * OISTermStructure_->discount(firstpaymentdate) + firstZeroCouponBond.NPV();
			alive = false;
		} else if ((t >= second_evaluationStart) && (t <= second_evaluation3) && (stock >= excerciselevel)) {
			price = plus * OISTermStructure_->discount(firstpaymentdate) + secondZeroCouponBond.NPV() + plus * OISTermStructure_->discount(secondpaymentdate);
			alive = false;
		} else if ((t >= second_evaluation4) && (t <= second_evaluationEnd) && (stock >= excerciselevel)) {
			price = plus * OISTermStructure_->discount(firstpaymentdate) + secondZeroCouponBond.NPV() + plus * OISTermStructure_->discount(secondpaymentdate);
			alive = false;
		} else if ((t >= third_evaluationStart) && (t <= third_evaluation2) && (stock >= excerciselevel)) {
			price = plus * OISTermStructure_->discount(firstpaymentdate) + thirdZeroCouponBond.NPV() + 2 * plus * OISTermStructure_->discount(thirdpaymentdate);
			alive = false;
		} else if ((t >= third_evaluation3) && (t <= third_evaluationEnd) && (stock >= excerciselevel)) {
			price = plus * OISTermStructure_->discount(firstpaymentdate) + thirdZeroCouponBond.NPV() + 2 * plus * OISTermStructure_->discount(thirdpaymentdate);
			alive = false;
		} else {
			step++;
			t += dt;
		}				
	}

	if (alive) {
		if (stock > excerciselevel) {
			price = plus * OISTermStructure_->discount(firstpaymentdate) + barrierZeroCouponBond.NPV() + 3 * plus * OISTermStructure_->discount(maturity_);
		} else {
			performance = (1 - stock / startinglevel);
			price =  plus * OISTermStructure_->discount(firstpaymentdate) + barrierZeroCouponBond.NPV() * (1 - performance);
		}
	}
	return price;

	/*
	Time time_tolerance = dt/2;
	for (Size step = 0; step < n -1; step ++) {
		t += dt;
		stock = path[step+1];
		price = plus * OISTermStructure_->discount(firstpaymentdate);

		//first evaluation period
		if ((fabs(t - first_evaluationStart) < time_tolerance) && (stock >= excerciselevel)) { 
			price += firstZeroCouponBond.NPV();
			return price;
		}
		if ((fabs(t - first_evaluation2) < time_tolerance) && (stock >= excerciselevel)) { 
			price += firstZeroCouponBond.NPV();
			return price;
		}
		if ((fabs(t - first_evaluation3) < time_tolerance) && (stock >= excerciselevel)) { 
			price += firstZeroCouponBond.NPV();
			return price;
		}
		if ((fabs(t - first_evaluation4) < time_tolerance) && (stock >= excerciselevel)) { 
			price += firstZeroCouponBond.NPV();
			return price;
		}
		if ((fabs(t - first_evaluationEnd) < time_tolerance) && (stock >= excerciselevel)) { 
			price += firstZeroCouponBond.NPV();
			return price;
		}

		//second evaluation period
		if ((fabs(t - second_evaluationStart) < time_tolerance) && (stock >= excerciselevel)) {
			price += (secondZeroCouponBond.NPV() + plus * OISTermStructure_->discount(secondpaymentdate));
			return price;
		}
		if ((fabs(t - second_evaluation2) < time_tolerance) && (stock >= excerciselevel)) {
			price += (secondZeroCouponBond.NPV() + plus * OISTermStructure_->discount(secondpaymentdate));
			return price;
		}
		if ((fabs(t - second_evaluation3) < time_tolerance) && (stock >= excerciselevel)) {
			price += (secondZeroCouponBond.NPV() + plus * OISTermStructure_->discount(secondpaymentdate));
			return price;
		}
		if ((fabs(t - second_evaluation4) < time_tolerance) && (stock >= excerciselevel)) {
			price += (secondZeroCouponBond.NPV() + plus * OISTermStructure_->discount(secondpaymentdate));
			return price;
		}
		if ((fabs(t - second_evaluationEnd) < time_tolerance) && (stock >= excerciselevel)) {
			price += (secondZeroCouponBond.NPV() + plus * OISTermStructure_->discount(secondpaymentdate));
			return price;
		}

		//third evaluation period
		if ((fabs(t - third_evaluationStart) < time_tolerance) && (stock >= excerciselevel)) {
			price += (thirdZeroCouponBond.NPV() + 2 * plus * OISTermStructure_->discount(thirdpaymentdate));
			return price;
		}
		if ((fabs(t - third_evaluation2) < time_tolerance) && (stock >= excerciselevel)) {
			price += (thirdZeroCouponBond.NPV() + 2 * plus * OISTermStructure_->discount(thirdpaymentdate));
			return price;
		}
		if ((fabs(t - third_evaluation3) < time_tolerance) && (stock >= excerciselevel)) {
			price += (thirdZeroCouponBond.NPV() + 2 * plus * OISTermStructure_->discount(thirdpaymentdate));
			return price;
		}
		if ((fabs(t - third_evaluation4) < time_tolerance) && (stock >= excerciselevel)) {
			price += (thirdZeroCouponBond.NPV() + 2 * plus * OISTermStructure_->discount(thirdpaymentdate));
			return price;
		}
		if ((fabs(t - third_evaluationEnd) < time_tolerance) && (stock >= excerciselevel)) {
			price += (thirdZeroCouponBond.NPV() + 2 * plus * OISTermStructure_->discount(thirdpaymentdate));
			return price;
		}

		//barrier evaluation period
		if (fabs(t - barrier_evaluation) < time_tolerance) {
			if (stock > excerciselevel) {
				price += (barrierZeroCouponBond.NPV() + 3 * plus * OISTermStructure_->discount(maturity_));
			}
			else {
				Real performance = (1 - stock / startinglevel);
				price += (barrierZeroCouponBond.NPV() * (1 - performance));
			}
			return price;
		}
	}*/
}