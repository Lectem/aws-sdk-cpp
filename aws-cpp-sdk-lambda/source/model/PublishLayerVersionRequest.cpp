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

#include <aws/lambda/model/PublishLayerVersionRequest.h>
#include <aws/core/utils/json/JsonSerializer.h>

#include <utility>

using namespace Aws::Lambda::Model;
using namespace Aws::Utils::Json;
using namespace Aws::Utils;

PublishLayerVersionRequest::PublishLayerVersionRequest() : 
    m_layerNameHasBeenSet(false),
    m_descriptionHasBeenSet(false),
    m_contentHasBeenSet(false),
    m_compatibleRuntimesHasBeenSet(false),
    m_licenseInfoHasBeenSet(false)
{
}

Aws::String PublishLayerVersionRequest::SerializePayload() const
{
  JsonValue payload;

  if(m_descriptionHasBeenSet)
  {
   payload.WithString("Description", m_description);

  }

  if(m_contentHasBeenSet)
  {
   payload.WithObject("Content", m_content.Jsonize());

  }

  if(m_compatibleRuntimesHasBeenSet)
  {
   Array<JsonValue> compatibleRuntimesJsonList(m_compatibleRuntimes.size());
   for(unsigned compatibleRuntimesIndex = 0; compatibleRuntimesIndex < compatibleRuntimesJsonList.GetLength(); ++compatibleRuntimesIndex)
   {
     compatibleRuntimesJsonList[compatibleRuntimesIndex].AsString(RuntimeMapper::GetNameForRuntime(m_compatibleRuntimes[compatibleRuntimesIndex]));
   }
   payload.WithArray("CompatibleRuntimes", std::move(compatibleRuntimesJsonList));

  }

  if(m_licenseInfoHasBeenSet)
  {
   payload.WithString("LicenseInfo", m_licenseInfo);

  }

  return payload.View().WriteReadable();
}




