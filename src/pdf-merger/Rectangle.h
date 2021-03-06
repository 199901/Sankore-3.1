/*
 * Copyright (C) 2012 Webdoc SA
 *
 * This file is part of Open-Sankoré.
 *
 * Open-Sankoré is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation, version 2,
 * with a specific linking exception for the OpenSSL project's
 * "OpenSSL" library (or with modified versions of it that use the
 * same license as the "OpenSSL" library).
 *
 * Open-Sankoré is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with Open-Sankoré; if not, see
 * <http://www.gnu.org/licenses/>.
 */


#if !defined Rectangle_h
#define Rectangle_h

#include "Transformation.h"

#include <vector>
#include <map>


namespace merge_lib
{
   class Object;

   class Rectangle
   {
   public:
      Rectangle(const char * rectangleName);

      Rectangle(const char * rectangleName, const std::string content);
      void appendRectangleToString(std::string & content, const char * delimeter);
      void updateRectangle(Object * objectWithRectangle, const char * delimeter);
      void setNewRectangleName(const char * newName);

      void recalculateInternalRectangleCoordinates(const PageTransformations & transformations);
      double getWidth();
      double getHeight();

      //members
      double x1, y1, x2, y2;
   private:
      //methods
      const std::string _getRectangleAsString(const char * delimeter);
      //members   
      const char * _rectangleName;
      TransformationMatrix _tm;
   };
}
#endif

