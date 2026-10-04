#include "head_notice_rules.h"

namespace totalrecruit
{
namespace
{
int IndexOf(const std::vector<HeadNotice>& notices, const CharKey& who)
{
    for (size_t i = 0; i < notices.size(); ++i)
    {
        if (notices[i].who == who)
        {
            return (int)i;
        }
    }
    return -1;
}
} // namespace

void HeadNoticeBoard::Raise(const CharKey& who, const void* subject, const std::string& name,
                            const std::string& text, float now, float seconds)
{
    // One per NPC: an existing notice is replaced, and the new one is the newest.
    const int existing = IndexOf(notices_, who);
    if (existing >= 0)
    {
        notices_.erase(notices_.begin() + existing);
    }
    else if ((int)notices_.size() >= kHeadNoticeCap)
    {
        notices_.erase(notices_.begin());
    }

    HeadNotice notice;
    notice.who = who;
    notice.subject = subject;
    notice.name = name;
    notice.text = text;
    notice.raisedAt = now;
    notice.expiresAt = now + seconds;
    notices_.push_back(notice);
}

bool HeadNoticeBoard::Dismiss(const CharKey& who)
{
    const int index = IndexOf(notices_, who);
    if (index < 0)
    {
        return false;
    }
    notices_.erase(notices_.begin() + index);
    return true;
}

int HeadNoticeBoard::Expire(float now)
{
    int gone = 0;
    for (size_t i = 0; i < notices_.size();)
    {
        if (notices_[i].expiresAt <= now)
        {
            notices_.erase(notices_.begin() + i);
            ++gone;
        }
        else
        {
            ++i;
        }
    }
    return gone;
}

const HeadNotice* HeadNoticeBoard::Find(const CharKey& who) const
{
    const int index = IndexOf(notices_, who);
    return index < 0 ? 0 : &notices_[index];
}

size_t HeadNoticeBoard::Count() const
{
    return notices_.size();
}

const HeadNotice& HeadNoticeBoard::At(size_t index) const
{
    return notices_[index];
}

void HeadNoticeBoard::Clear()
{
    notices_.clear();
}

NoticeFrame DecideNoticeFrame(const NoticeFrameInput& input)
{
    // Selecting the NPC hands the head to the normal readout card for good.
    if (input.selectedNow)
    {
        return NoticeFrame_Dismiss;
    }
    if (!input.placed || input.menuOpen || input.nativeWindowOpen)
    {
        return NoticeFrame_Hide;
    }
    if (input.cardVisible && RectsOverlap(input.card, input.notice))
    {
        return NoticeFrame_Hide;
    }
    return NoticeFrame_Draw;
}

bool RectsOverlap(const ScreenRect& a, const ScreenRect& b)
{
    if (a.width <= 0 || a.height <= 0 || b.width <= 0 || b.height <= 0)
    {
        return false;
    }
    return a.left < b.left + b.width && b.left < a.left + a.width &&
           a.top < b.top + b.height && b.top < a.top + a.height;
}

float ClampHeadNoticeSeconds(float seconds)
{
    if (!(seconds >= kHeadNoticeSecondsMin))
    {
        return kHeadNoticeSecondsMin;
    }
    if (seconds > kHeadNoticeSecondsMax)
    {
        return kHeadNoticeSecondsMax;
    }
    return seconds;
}
} // namespace totalrecruit
