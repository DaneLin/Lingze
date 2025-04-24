#pragma once

#include "LingzeVK.h"

namespace lz
{
	struct TimestampQuery
	{
		TimestampQuery(vk::PhysicalDevice physical_device, vk::Device logical_device, uint32_t max_timestamp_count);

		void reset_query_pool(vk::CommandBuffer command_buffer);

		void add_timestamp(vk::CommandBuffer command_buffer, size_t timestamp_name,
		                  vk::PipelineStageFlagBits pipeline_stage);

		struct QueryResult
		{
			struct TimestampData
			{
				size_t timestamp_name;
				double time;
			};

			const TimestampData* data;
			size_t size;
		};

		QueryResult query_results(vk::Device logical_device);

	private:
		std::vector<uint64_t> query_results_;
		std::vector<QueryResult::TimestampData> timestamp_datas_;
		vk::UniqueQueryPool query_pool_;
		uint32_t curr_timestamp_index_;
		float timestamp_period_;
		uint64_t timestamp_mask_;
	};
}
