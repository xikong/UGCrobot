#ifndef TEMPLATES_BUILDER_H_IS9OZD1Q
#define TEMPLATES_BUILDER_H_IS9OZD1Q

#include <string>
#include "tinyxml2.h"
#include <boost/utility.hpp>
#include "../../../fast/mfast/field_instructions.h"
#include "../../../fast/mfast/xml_parser/dynamic_templates_description.h"
#include "../../../fast/mfast/xml_parser/field_builder.h"

namespace mfast
{
  namespace xml_parser
  {

    class templates_builder
      : public XMLVisitor
      , public boost::base_from_member<type_map_t>
      , public field_builder_base
    {
    public:
      templates_builder(dynamic_templates_description* definition,
                        const char*                    cpp_ns,
                        template_registry*             registry);

      virtual bool  VisitEnter (const XMLElement & element, const XMLAttribute* attr);
      virtual bool  VisitExit (const XMLElement & element);


      virtual std::size_t num_instructions() const;
      virtual void add_instruction(const field_instruction*);
      void add_template(const char* ns, template_instruction* inst);

      virtual const char* name() const
      {
        return cpp_ns_;
      }

    protected:
      dynamic_templates_description* definition_;
      const char* cpp_ns_;
      std::deque<const template_instruction*> templates_;
      const template_instruction template_instruction_prototype_;
      const group_field_instruction group_field_instruction_prototype_;
      const sequence_field_instruction sequence_field_instruction_prototype_;
      const enum_field_instruction enum_field_instruction_prototype_;
    };

  } /* coder */

} /* mfast */


#endif /* end of include guard: TEMPLATES_BUILDER_H_IS9OZD1Q */
