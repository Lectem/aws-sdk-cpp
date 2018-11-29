﻿/*
* Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License").
* You may not use this file except in compliance with the License.
* A copy of the License is located at
*
*  http://aws.amazon.com/apache2.0
*
* or in the "license" file accompanying this file. This file is distributed
* on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
* express or implied. See the License for the specific language governing
* permissions and limitations under the License.
*/

#include <aws/s3/model/SelectObjectContentHandler.h>
#include <aws/s3/S3ErrorMarshaller.h>
#include <aws/core/client/CoreErrors.h>
#include <aws/core/utils/event/EventStreamErrors.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/xml/XmlSerializer.h>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Event;
using namespace Aws::Utils::Xml;

namespace Aws
{
namespace S3
{
namespace Model
{
    using namespace Aws::Client;

    static const char SELECTOBJECTCONTENT_HANDLER_CLASS_TAG[] = "SelectObjectContentHandler";

    SelectObjectContentHandler::SelectObjectContentHandler() : EventStreamHandler()
    {
        m_onRecordsEvent = [&](RecordsEvent&)
        {
            AWS_LOGSTREAM_TRACE(SELECTOBJECTCONTENT_HANDLER_CLASS_TAG, "RecordsEvent received.");
        };

        m_onStatsEvent = [&](const StatsEvent&)
        {
            AWS_LOGSTREAM_TRACE(SELECTOBJECTCONTENT_HANDLER_CLASS_TAG, "StatsEvent received.");
        };

        m_onProgressEvent = [&](const ProgressEvent&)
        {
            AWS_LOGSTREAM_TRACE(SELECTOBJECTCONTENT_HANDLER_CLASS_TAG, "ProgressEvent received.");
        };

        m_onContinuationEvent = [&]()
        {
            AWS_LOGSTREAM_TRACE(SELECTOBJECTCONTENT_HANDLER_CLASS_TAG, "ContinuationEvent received.");
        };

        m_onEndEvent = [&]()
        {
            AWS_LOGSTREAM_TRACE(SELECTOBJECTCONTENT_HANDLER_CLASS_TAG, "EndEvent received.");
        };

        m_onError = [&](const AWSError<S3Errors>& error)
        {
            AWS_LOGSTREAM_TRACE(SELECTOBJECTCONTENT_HANDLER_CLASS_TAG, "S3 Errors received, " << error);
        };
    }

    void SelectObjectContentHandler::OnEvent()
    {
        using namespace Aws::Utils::Event;

        // Handler internal error during event stream decoding.
        if (!*this)
        {
            AWSError<CoreErrors> error = EventStreamErrorsMapper::GetAwsErrorForEventStreamError(GetInternalError());
            error.SetMessage(GetEventPayloadAsString());
            m_onError(AWSError<S3Errors>(error));
            return;
        }

        auto headers = GetEventHeaders();
        auto messageTypeHeaderIter = headers.find(MESSAGE_TYPE_HEADER);
        if (messageTypeHeaderIter == headers.end())
        {
            AWS_LOGSTREAM_WARN(SELECTOBJECTCONTENT_HANDLER_CLASS_TAG, "Header: " << MESSAGE_TYPE_HEADER << " not found in the message.");
            return;
        }

        switch (Message::GetMessageTypeForName(messageTypeHeaderIter->second.GetEventHeaderValueAsString()))
        {
        case Message::MessageType::EVENT:
            HandleEventInMessage();
            break;
        case Message::MessageType::REQUEST_LEVEL_ERROR:
        {
            HandleErrorInMessage();
            break;
        }
        default:
            AWS_LOGSTREAM_WARN(SELECTOBJECTCONTENT_HANDLER_CLASS_TAG,
                "Unexpected message type: " << messageTypeHeaderIter->second.GetEventHeaderValueAsString());
            break;
        }
    }

    void SelectObjectContentHandler::HandleEventInMessage()
    {
        auto headers = GetEventHeaders();
        auto eventTypeHeaderIter = headers.find(EVENT_TYPE_HEADER);
        if (eventTypeHeaderIter == headers.end())
        {
            AWS_LOGSTREAM_WARN(SELECTOBJECTCONTENT_HANDLER_CLASS_TAG, "Header: " << EVENT_TYPE_HEADER << " not found in the message.");
            return;
        }
        switch (SelectObjectContentEventMapper::GetSelectObjectContentEventTypeForName(eventTypeHeaderIter->second.GetEventHeaderValueAsString()))
        {
        case SelectObjectContentEventType::RECORDS:
        {
            RecordsEvent event(GetEventPayloadWithOwnership());
            m_onRecordsEvent(event);
            break;
        }
        case SelectObjectContentEventType::STATS:
        {
            auto xmlDoc = XmlDocument::CreateFromXmlString(GetEventPayloadAsString());
            if (!xmlDoc.WasParseSuccessful())
            {
                AWS_LOGSTREAM_WARN(SELECTOBJECTCONTENT_HANDLER_CLASS_TAG, "Unable to generate a proper StatsEvent object from the response in XML format.");
                break;
            }

            m_onStatsEvent(StatsEvent(xmlDoc.GetRootElement()));
            break;
        }
        case SelectObjectContentEventType::PROGRESS:
        {
            auto xmlDoc = XmlDocument::CreateFromXmlString(GetEventPayloadAsString());
            if (!xmlDoc.WasParseSuccessful())
            {
                AWS_LOGSTREAM_WARN(SELECTOBJECTCONTENT_HANDLER_CLASS_TAG, "Unable to generate a proper ProgressEvent object from the response in XML format.");
                break;
            }

            m_onProgressEvent(ProgressEvent(xmlDoc.GetRootElement()));
            break;
        }
        case SelectObjectContentEventType::CONT:
            m_onContinuationEvent();
            break;
        case SelectObjectContentEventType::END:
            m_onEndEvent();
            break;
        default:
            AWS_LOGSTREAM_WARN(SELECTOBJECTCONTENT_HANDLER_CLASS_TAG,
                "Unexpected event type: " << headers[EVENT_TYPE_HEADER].GetEventHeaderValueAsString());
            break;
        }
    }

    void SelectObjectContentHandler::HandleErrorInMessage()
    {
        auto headers = GetEventHeaders();
        Aws::String errorCode = "";
        Aws::String errorMessage = "";
        auto errorCodeHeaderIter = headers.find(ERROR_CODE_HEADER);
        if (errorCodeHeaderIter == headers.end())
        {
            AWS_LOGSTREAM_WARN(SELECTOBJECTCONTENT_HANDLER_CLASS_TAG,
                "Header: " << ERROR_CODE_HEADER << " not found in the message.");
        }
        else
        {
            errorCode = errorCodeHeaderIter->second.GetEventHeaderValueAsString();
        }
        auto errorMessageHeaderIter = headers.find(ERROR_MESSAGE_HEADER);
        if (errorMessageHeaderIter == headers.end())
        {
            AWS_LOGSTREAM_WARN(SELECTOBJECTCONTENT_HANDLER_CLASS_TAG,
                "Header: " << ERROR_MESSAGE_HEADER << " not found in the message.");
        }
        else
        {
            errorMessage = errorMessageHeaderIter->second.GetEventHeaderValueAsString();
        }

        S3ErrorMarshaller errorMarshaller;
        AWSError<CoreErrors> error;

        if (errorCode.empty())
        {
            error = AWSError<CoreErrors>(CoreErrors::UNKNOWN, "", errorMessage, false);
        }
        else
        {
            error = errorMarshaller.FindErrorByName(errorMessage.c_str());
            if (error.GetErrorType() != CoreErrors::UNKNOWN)
            {
                AWS_LOGSTREAM_WARN(SELECTOBJECTCONTENT_HANDLER_CLASS_TAG, "Encountered AWSError '" << errorCode.c_str() << "': " << errorMessage.c_str());
                error.SetExceptionName(errorCode);
                error.SetMessage(errorMessage);
            }
            else
            {
                AWS_LOGSTREAM_WARN(SELECTOBJECTCONTENT_HANDLER_CLASS_TAG, "Encountered Unknown AWSError '" << errorCode.c_str() << "': " << errorMessage.c_str());
                error = AWSError<CoreErrors>(CoreErrors::UNKNOWN, errorCode, "Unable to parse ExceptionName: " + errorCode + " Message: " + errorMessage, false);
            }
        }

        m_onError(AWSError<S3Errors>(error));
    }

namespace SelectObjectContentEventMapper
{
    static const int RECORDS_HASH = Aws::Utils::HashingUtils::HashString("Records");
    static const int STATS_HASH = Aws::Utils::HashingUtils::HashString("Stats");
    static const int PROGRESS_HASH = Aws::Utils::HashingUtils::HashString("Progress");
    static const int CONT_HASH = Aws::Utils::HashingUtils::HashString("Cont");
    static const int END_HASH = Aws::Utils::HashingUtils::HashString("End");

    SelectObjectContentEventType GetSelectObjectContentEventTypeForName(const Aws::String& name)
    {
        int hashCode = Aws::Utils::HashingUtils::HashString(name.c_str());
        if (hashCode == RECORDS_HASH)
        {
            return SelectObjectContentEventType::RECORDS;
        }
        else if (hashCode == STATS_HASH)
        {
            return SelectObjectContentEventType::STATS;
        }
        else if (hashCode == PROGRESS_HASH)
        {
            return SelectObjectContentEventType::PROGRESS;
        }
        else if (hashCode == CONT_HASH)
        {
            return SelectObjectContentEventType::CONT;
        }
        else if (hashCode == END_HASH)
        {
            return SelectObjectContentEventType::END;
        }
        return SelectObjectContentEventType::UNKNOWN;
    }

    Aws::String GetNameForSelectObjectContentEventType(SelectObjectContentEventType value)
    {
        switch (value)
        {
        case SelectObjectContentEventType::RECORDS:
            return "Records";
        case SelectObjectContentEventType::STATS:
            return "Stats";
        case SelectObjectContentEventType::PROGRESS:
            return "Progress";
        case SelectObjectContentEventType::CONT:
            return "Cont";
        case SelectObjectContentEventType::END:
            return "End";
        default:
            return "Unknown";
        }
    }
} // namespace SelectObjectContentEventMapper
} // namespace Model
} // namespace S3
} // namespace Aws