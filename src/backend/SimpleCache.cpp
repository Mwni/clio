#include <backend/SimpleCache.h>
namespace Backend {

void
SimpleCache::update(
    std::vector<LedgerObject> const& objs,
    uint32_t seq,
    bool isBackground)
{
    if (!enabled_)
        return;
    std::unique_lock lck{mtx_};
    if (seq > latestSeq_)
    {
        assert(seq == latestSeq_ + 1 || latestSeq_ == 0);
        latestSeq_ = seq;
    }
    for (auto const& obj : objs)
    {
        if (obj.blob.size())
        {
            if (isBackground && deletes_.count(obj.key))
                continue;
            auto& e = map_[obj.key];
            if (seq > e.seq)
            {
                e = {seq, obj.blob};
            }
        }
        else
        {
            map_.erase(obj.key);
            if (!full_ && !isBackground)
                deletes_.insert(obj.key);
        }
    }
}
std::optional<LedgerObject>
SimpleCache::getSuccessor(ripple::uint256 const& key, uint32_t seq) const
{
    if (!enabled_ || !full_)
        return {};
    std::shared_lock{mtx_};
    if (seq != latestSeq_)
        return {};
    auto e = map_.upper_bound(key);
    if (e == map_.end())
        return {};
    return {{e->first, e->second.blob}};
}
std::optional<LedgerObject>
SimpleCache::getPredecessor(ripple::uint256 const& key, uint32_t seq) const
{
    if (!enabled_ || !full_)
        return {};
    std::shared_lock lck{mtx_};
    if (seq != latestSeq_)
        return {};
    auto e = map_.lower_bound(key);
    if (e == map_.begin())
        return {};
    --e;
    return {{e->first, e->second.blob}};
}
std::optional<Blob>
SimpleCache::get(ripple::uint256 const& key, uint32_t seq) const
{
    if (!enabled_ || seq > latestSeq_)
        return {};
    std::shared_lock lck{mtx_};
    auto e = map_.find(key);
    if (e == map_.end())
        return {};
    if (seq < e->second.seq)
        return {};
    return {e->second.blob};
}

void
SimpleCache::setFull()
{
    full_ = true;
    std::unique_lock lck{mtx_};
    deletes_.clear();
}
void
SimpleCache::disable()
{
    enabled_ = false;
}
void
SimpleCache::enable()
{
    enabled_ = true;
}
bool
SimpleCache::isEnabled()
{
    return enabled_;
}
bool
SimpleCache::isFull()
{
    return full_;
}
size_t
SimpleCache::size()
{
    std::shared_lock lck{mtx_};
    return map_.size();
}
}  // namespace Backend
