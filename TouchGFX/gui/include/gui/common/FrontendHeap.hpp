#ifndef FRONTENDHEAP_HPP
#define FRONTENDHEAP_HPP

#include <gui_generated/common/FrontendHeapBase.hpp>
/* Screen2-Typen fuer TypeList-Groessenberechnung */
#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>

class FrontendHeap : public FrontendHeapBase
{
public:
    /* Alle user-definierten View-Typen */
    typedef touchgfx::meta::TypeList< Screen2View,
                            touchgfx::meta::Nil
                            > UserDefinedViewTypes;

    /* Alle user-definierten Presenter-Typen */
    typedef touchgfx::meta::TypeList< Screen2Presenter,
                            touchgfx::meta::Nil
                            > UserDefinedPresenterTypes;

    /* Transitions (keine zusaetzlichen) */
    typedef touchgfx::meta::TypeList< touchgfx::meta::Nil,
                            touchgfx::meta::Nil
                            > UserDefinedTransitionTypes;

    typedef touchgfx::meta::select_type_maxsize< UserDefinedViewTypes >::type MaxUserViewType;

    typedef touchgfx::meta::TypeList< MaxGeneratedViewType,
                touchgfx::meta::TypeList< MaxUserViewType,
                            touchgfx::meta::Nil
                            > > CombinedViewTypes;

    typedef touchgfx::meta::select_type_maxsize< CombinedViewTypes >::type MaxViewType;

    typedef touchgfx::meta::select_type_maxsize< UserDefinedPresenterTypes >::type MaxUserPresenterType;

    typedef touchgfx::meta::TypeList< MaxGeneratedPresenterType,
            touchgfx::meta::TypeList< MaxUserPresenterType,
                            touchgfx::meta::Nil
                            > > CombinedPresenterTypes;

    typedef touchgfx::meta::select_type_maxsize< CombinedPresenterTypes >::type MaxPresenterType;

    typedef touchgfx::meta::select_type_maxsize< UserDefinedTransitionTypes >::type MaxUserTransitionType;

    typedef touchgfx::meta::TypeList< MaxGeneratedTransitionType,
            touchgfx::meta::TypeList< MaxUserTransitionType,
                            touchgfx::meta::Nil
                            > > CombinedTransitionTypes;

    typedef touchgfx::meta::select_type_maxsize< CombinedTransitionTypes >::type MaxTransitionType;

    static FrontendHeap& getInstance()
    {
        static FrontendHeap instance;
        return instance;
    }

    touchgfx::Partition< CombinedPresenterTypes, 1 > presenters;
    touchgfx::Partition< CombinedViewTypes,      1 > views;
    touchgfx::Partition< CombinedTransitionTypes, 1 > transitions;
    Model model;
    FrontendApplication app;

private:
    FrontendHeap() : FrontendHeapBase(presenters, views, transitions, app),
                     app(model, *this)
    {
        gotoStartScreen(app);
    }
};

#endif // FRONTENDHEAP_HPP
