#include "stdafx.h"
#include "GraphicsTimer.h"

namespace happy
{
	GraphicsTimer::GraphicsTimer(ID3D11Device& device, ID3D11DeviceContext& context)
		: m_pDevice(&device)
		, m_pContext(&context)
	{
		{
			D3D11_QUERY_DESC desc;
			desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
			desc.MiscFlags = 0;
			m_pDevice->CreateQuery(&desc, &m_QueriesDisjoint[0]);
			m_pDevice->CreateQuery(&desc, &m_QueriesDisjoint[1]);
			m_pCurrentDisjoint = m_QueriesDisjoint[0].Get();
			m_pRunningDisjoint = m_QueriesDisjoint[1].Get();
		}

		{
			auto &q = m_RootQuery;

			D3D11_QUERY_DESC desc;
			desc.Query = D3D11_QUERY_TIMESTAMP;
			desc.MiscFlags = 0;
			for (int i=0; i<4; ++i)
				m_pDevice->CreateQuery(&desc, &q.m_Q[i]);
			q.m_pCurrentBegin = q.m_Q[0].Get();
			q.m_pCurrentEnd = q.m_Q[1].Get();
			q.m_pRunningBegin = q.m_Q[2].Get();
			q.m_pRunningEnd = q.m_Q[3].Get();
			q.m_avg.resize(10, 0);
			q.m_ptr = 0;
			q.m_Counter = 0;
			q.m_Parent = nullptr;
			q.m_Tag = "Frame render";
		}

		m_DumpFn = [](const char *str)
		{
			OutputDebugStringA(str);
			OutputDebugStringA("\n");
		};
	}

	void GraphicsTimer::setLogDump(std::function<void(const char *)> dumpFn)
	{
		m_DumpFn = dumpFn;
	}

	void GraphicsTimer::begin()
	{
		m_pContext->Begin(m_pRunningDisjoint);
		m_pContext->End(m_RootQuery.m_pRunningBegin);
		m_pCurrentQuery = &m_RootQuery;
		m_pCurrentQuery->reset();
	}

	ID3D11Query* GraphicsTimer::beginZone(const char *tag)
	{
		auto *curr = m_pCurrentQuery;
		if (curr->m_Counter >= curr->m_SubQueries.size())
		{
			curr->m_SubQueries.emplace_back();
			auto &q = curr->m_SubQueries.back();

			D3D11_QUERY_DESC desc;
			desc.Query = D3D11_QUERY_TIMESTAMP;
			desc.MiscFlags = 0;
			for (int i=0; i<4; ++i)
				m_pDevice->CreateQuery(&desc, &q.m_Q[i]);
			q.m_pCurrentBegin = q.m_Q[0].Get();
			q.m_pCurrentEnd = q.m_Q[1].Get();
			q.m_pRunningBegin = q.m_Q[2].Get();
			q.m_pRunningEnd = q.m_Q[3].Get();
			q.m_avg.resize(10, 0);
			q.m_ptr = 0;
			q.m_Counter = 0;
			q.m_Parent = curr;
		}

		query *q;
		q = &curr->m_SubQueries[curr->m_Counter];
		q->m_Tag = tag;
		q->m_Parent = curr;
		m_pCurrentQuery = q;

		curr->m_Counter++;

		m_pContext->End(q->m_pRunningBegin);
		return q->m_pRunningEnd;
	}

	void GraphicsTimer::endZone(ID3D11Query *query)
	{
		m_pContext->End(query);
		m_pCurrentQuery = m_pCurrentQuery->m_Parent;
	}

	void GraphicsTimer::query::reset()
	{
		m_Counter = 0;
		for (auto &s : m_SubQueries) s.reset();
	}

	void GraphicsTimer::query::handle(ID3D11DeviceContext *context, UINT64 frequency, BOOL disjoint)
	{
		//===================================================
		// Handle sub timing
		//===================================================
		for (unsigned i = 0; i < m_SubQueries.size(); ++i)
		{
			m_SubQueries[i].handle(context, frequency, disjoint);
		}

		//===================================================
		// Handle own timing
		//===================================================
		UINT64 queriedBegin = 0, queriedEnd = 0;
		context->GetData(m_pCurrentBegin, &queriedBegin, sizeof(UINT64), 0);
		context->GetData(m_pCurrentEnd, &queriedEnd, sizeof(UINT64), 0);

		if (!disjoint)
		{
			m_avg[m_ptr] = double(queriedEnd - queriedBegin) / double(frequency) * 1000000.0f;
			m_ptr = (m_ptr + 1) % m_avg.size();
		}

		std::swap(m_pCurrentBegin, m_pRunningBegin);
		std::swap(m_pCurrentEnd, m_pRunningEnd);
	}

	void GraphicsTimer::query::report(std::ostream &os, int level)
	{
		double avg = 0;
		for (auto &s : m_avg)
		{
			avg += s;
		}
		avg /= (float)m_avg.size();

		os << "|| ";
		for (int i = 0; i < level; ++i) os << "  ";
		os << "(" << round(avg*10.0) / 10.0 << "us) " << m_Tag << std::endl;

		for (auto &s : m_SubQueries)
		{
			s.report(os, level + 1);
		}
	}

	void GraphicsTimer::end()
	{
		m_pContext->End(m_RootQuery.m_pRunningEnd);
		m_pContext->End(m_pRunningDisjoint);

		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint;
		while (S_FALSE == m_pContext->GetData(m_pCurrentDisjoint, &disjoint, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0))
		{
		}
		
		m_RootQuery.handle(m_pContext, disjoint.Frequency, disjoint.Disjoint);

		std::swap(m_pCurrentDisjoint, m_pRunningDisjoint);

		if ((++m_Frames % 450) == 0 && m_DumpFn)
		{
			std::stringstream report;
			report << "//=GPU FRAME TIMINGS====================" << std::endl;
			report << "|| " << std::endl;
			m_RootQuery.report(report, 1);

			while (!report.eof())
			{
				char linebuf[2048];
				memset(linebuf, 0, 2048);
				report.getline(linebuf, 2048);

				m_DumpFn(linebuf);
			}
		}
	}
}