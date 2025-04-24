#include "TimestampQuery.h"

namespace lz
{
	TimestampQuery::TimestampQuery(vk::PhysicalDevice physical_device, vk::Device logical_device,
		uint32_t max_timestamp_count)
	{
		const auto query_pool_info = vk::QueryPoolCreateInfo()
		                     .setQueryType(vk::QueryType::eTimestamp)
		                     .setQueryCount(max_timestamp_count);
		this->query_pool_ = logical_device.createQueryPoolUnique(query_pool_info);
		this->timestamp_datas_.resize(max_timestamp_count);
		this->query_results_.resize(max_timestamp_count);
		this->curr_timestamp_index_ = 0;
		this->timestamp_period_ = physical_device.getProperties().limits.timestampPeriod;
	}

	void TimestampQuery::reset_query_pool(vk::CommandBuffer command_buffer)
	{
		command_buffer.resetQueryPool(query_pool_.get(), 0, static_cast<uint32_t>(timestamp_datas_.size()));
		curr_timestamp_index_ = 0;
	}

	void TimestampQuery::add_timestamp(vk::CommandBuffer command_buffer, size_t timestamp_name,
		vk::PipelineStageFlagBits pipeline_stage)
	{
		assert(curr_timestamp_index_ < timestamp_datas_.size());
		command_buffer.writeTimestamp(pipeline_stage, query_pool_.get(), curr_timestamp_index_);
		timestamp_datas_[curr_timestamp_index_].timestamp_name = timestamp_name;
		curr_timestamp_index_++;
	}

	TimestampQuery::QueryResult TimestampQuery::query_results(vk::Device logical_device)
	{
		std::fill(query_results_.begin(), query_results_.end(), 0);
		const auto query_res = logical_device.getQueryPoolResults(query_pool_.get(), 0, curr_timestamp_index_,
		                                                  query_results_.size() * sizeof(std::uint64_t),
		                                                  query_results_.data(), sizeof(std::uint64_t),
		                                                  vk::QueryResultFlagBits::e64 |
		                                                  vk::QueryResultFlagBits::eWait);
		//assert(queryRes == vk::Result::eSuccess);
		if (query_res == vk::Result::eSuccess)
		{
			for (uint32_t timestamp_index = 0; timestamp_index < curr_timestamp_index_; timestamp_index++)
			{
				timestamp_datas_[timestamp_index].time = (query_results_[timestamp_index] - query_results_[0]) * double(
					timestamp_period_ / 1e9); //in seconds
			}
		}

		QueryResult res;
		res.data = timestamp_datas_.data();
		res.size = curr_timestamp_index_;
		return res;
	}
}
