#include "AnimGraphMotionEventAction.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include "Allocators.h"

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphMotionEventAction, AnimGraphAllocator, 0);

    void AnimGraphMotionEventAction::Reflect(AZ::ReflectContext * context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context)) {
            serializeContext->Class<AnimGraphMotionEventAction, AnimGraphTriggerAction>()
                ->Version(1)
                ->Field("m_event", &AnimGraphMotionEventAction::m_event)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext()) {

                editContext->Class<AnimGraphMotionEventAction>("Send Motion Event Action", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->DataElement(0, &AnimGraphMotionEventAction::m_event,
                        "Event data",
                        "The data to be sent when this action fires")
                    ;
            }
        }
    }

    AnimGraphMotionEventAction::AnimGraphMotionEventAction()
    {
    }

    AnimGraphMotionEventAction::AnimGraphMotionEventAction(AnimGraph* animGraph)
        : AnimGraphMotionEventAction()
    {
        InitAfterLoading(animGraph);
    }

    AnimGraphMotionEventAction::~AnimGraphMotionEventAction()
    {
    }

    void AnimGraphMotionEventAction::Reinit()
    {
        if (!m_animGraph) return;

        // NB: we copy the event data shared ptrs because the serializer looks at m_event
        EventDataSet evData = m_event.GetEventDatas();
        m_wrappedEvent = MotionEvent(0.f, AZStd::move(evData));
    }

    bool AnimGraphMotionEventAction::InitAfterLoading(AnimGraph * animGraph)
    {
        if (!AnimGraphTriggerAction::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    template<class ToStringRec, class It>
    static AZStd::string RangeToString(It begin, It end, ToStringRec toString)
    {
        AZStd::string out = "{";
        if (begin != end)
        {
            out += toString(*begin);
            for (auto it = begin++; it != end; ++it)
            {
                out += ", " + toString(*it);
            }
        }
        out += "}";
        return out;
    }

    static AZStd::string EventToString(const Event& evs)
    {
        const auto& evData = evs.GetEventDatas();
        return RangeToString(evData.begin(), evData.end(), [](const auto& ev) { return ev->ToString(); });
    }

    void AnimGraphMotionEventAction::GetSummary(AZStd::string* outResult) const
    {
        *outResult = AZStd::string::format(
            "%s: Event datas: %s", 
            RTTI_GetTypeName(),
            EventToString(m_event).c_str()
        );
    }

    void AnimGraphMotionEventAction::GetTooltip(AZStd::string* outResult) const
    {
        return GetSummary(outResult);
    }

    const char * AnimGraphMotionEventAction::GetPaletteName() const
    {
        return "Motion Event Action";
    }

    void AnimGraphMotionEventAction::TriggerAction(AnimGraphInstance* animGraphInstance) const
    {
        if (!animGraphInstance)
        {
            AZ_Warning("AnimGraphMotionEventAction", false, "No AnimGraphInstance");
            return;
        }

        auto rootNode = animGraphInstance->GetRootNode();
        if (!rootNode)
        {
            AZ_Warning("AnimGraphMotionEventAction", false, "No AnimGraphInstance root node");
            return;
        }

        auto uniqueData = rootNode->FindOrCreateUniqueNodeData(animGraphInstance);
        if (!uniqueData)
        {
            AZ_Warning("AnimGraphMotionEventAction", false, "No AnimGraphInstance root node unique data");
            return;
        }

        auto refCountedData = uniqueData->GetRefCountedData();
        if (!refCountedData)
        {
            rootNode->RequestRefDatas(animGraphInstance);
            refCountedData = uniqueData->GetRefCountedData();
        }

        if (!refCountedData)
        {
            AZ_Warning("AnimGraphMotionEventAction", false, "No AnimGraphInstance root node unique data ref counted data");
            return;
        }

        EventInfo eventInfo(
            animGraphInstance->GetGlobalPlaytime(),
            animGraphInstance->GetActorInstance(),
            nullptr,
            const_cast<MotionEvent*>(&m_wrappedEvent),
            EventInfo::EventState::START
        );
        // NB: the emitter pointer MUST be valid (it is accessed without checking for null)
        eventInfo.m_emitter = rootNode;

        refCountedData->GetEventBuffer().AddEvent(eventInfo);
    }
}
