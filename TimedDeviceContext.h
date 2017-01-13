#pragma once

namespace happy
{
	class TimedDeviceContext
	{
		friend class RenderingContext;

		TimedDeviceContext(ID3D11DeviceContext *context, const char *perfZone, function<void()> cb)
			: m_pContext(context)
			, m_pRef(new int)
			, m_Callback(cb)
		{
			(*m_pRef) = 1;
		}

		ID3D11DeviceContext *m_pContext;

		int *m_pRef;

		function<void()> m_Callback;

	public:
		TimedDeviceContext(const TimedDeviceContext& other)
			: m_pContext(other.m_pContext)
			, m_pRef(other.m_pRef)
			, m_Callback(other.m_Callback)
		{
			(*m_pRef)++;
		}

		TimedDeviceContext& operator=(const TimedDeviceContext& other)
		{
			m_pContext = other.m_pContext;
			m_pRef = other.m_pRef;
			m_Callback = other.m_Callback;
			(*m_pRef)++;
		}

		~TimedDeviceContext()
		{
			(*m_pRef)--;
			if ((*m_pRef) == 0)
			{
				m_Callback();
				delete m_pRef;
			}
		}

		operator ID3D11DeviceContext*() const
		{
			return m_pContext;
		}

		ID3D11DeviceContext& operator*() const
		{
			return *m_pContext;
		}

		ID3D11DeviceContext* operator->() const
		{
			return m_pContext;
		}
	};
}